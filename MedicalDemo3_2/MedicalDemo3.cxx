// Derived from VTK/Examples/Cxx/Medical3.cxx
// This example reads a volume dataset, extracts two isosurfaces that
// represent the skin and bone, creates three orthogonal planes
// (sagittal, axial, coronal), and displays them.
//

#include <array>
#include <vector>
#include <algorithm>
#include <vtkObject.h>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkImageActor.h>
#include <vtkImageMapToColors.h>
#include <vtkImageMapper3D.h>
#include <vtkLookupTable.h>
#include <vtkMetaImageReader.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkStripper.h>
#include <vtkVersion.h>
#include <vtkInteractorStyleTrackballActor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkImageData.h>
#include <vtkObjectFactory.h>
#include <vtkPropPicker.h>
#include <vtkCellPicker.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkShortArray.h>
#include <vtkPointData.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkPlaneSource.h>

// vtkFlyingEdges3D was introduced in VTK >= 8.2
#if VTK_MAJOR_VERSION >= 9 || (VTK_MAJOR_VERSION >= 8 && VTK_MINOR_VERSION >= 2)
#define USE_FLYING_EDGES
#else
#undef USE_FLYING_EDGES
#endif

#ifdef USE_FLYING_EDGES
#include <vtkFlyingEdges3D.h>
#else
#include <vtkMarchingCubes.h>
#endif


static int		length = 0;
static double	total_vector[6][3];
static double	pColor[3] = { 0.0, 0.0, 0.0 };
const double	increamentXYZ = 1;
const int		NUMOFPLANES = 6;

class vtkCustomInteractorStyle : public vtkInteractorStyleTrackballActor
{
public:
	static vtkCustomInteractorStyle* New();
	vtkTypeMacro(vtkCustomInteractorStyle, vtkInteractorStyleTrackballActor);


	vtkCustomInteractorStyle()
	{
		this->ActorStyle = vtkSmartPointer<vtkInteractorStyleTrackballActor>::New();
		this->CameraStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
		this->CurrentStyle = this->ActorStyle;
	}

	void SetRenderer(vtkRenderer* renderer) { this->Renderer = renderer; }

	virtual void OnLeftButtonDown() override
	{
		if (this->CurrentStyle == this->ActorStyle) {
			int clickPos[2];
			this->Interactor->GetEventPosition(clickPos);
			vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();
			picker->Pick(clickPos[0], clickPos[1], 0, this->Renderer);
			this->m_pTarget = vtkActor::SafeDownCast(picker->GetActor());

			if (this->m_pTarget)
			{
				this->Interactor->GetEventPosition(this->LastPos);

				for (int i = 0; i < 6; i++ )
				{
					if (this->m_pTarget == m_pActors[i])
					{
						m_Moving[i] = true;
						std::cout << i << " has been selected " << std::endl;
					}
					else {
						m_Moving[i] = false;
						std::cout << i << " Passed " << std::endl;
					}
				}
				std::cout << std::endl;
			}
			else {
				vtkInteractorStyleTrackballActor::OnLeftButtonDown();
			}
		}
		else
		{
			this->CurrentStyle->OnLeftButtonDown();
		}
	}

	virtual void OnRightButtonDown() override
	{
		if (this->CurrentStyle == this->ActorStyle) {
			int clickPos[2];
			this->Interactor->GetEventPosition(clickPos);
			vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();
			picker->Pick(clickPos[0], clickPos[1], 0, this->Renderer);
			this->m_pTarget = vtkActor::SafeDownCast(picker->GetActor());

			if (this->m_pTarget)
			{
				m_bMovingAllActors = true;
			}
		}
		else
		{
			this->CurrentStyle->OnRightButtonDown();
		}
	}


	virtual void OnMouseMove() override
	{
		if (this->CurrentStyle == this->ActorStyle) {
			if (m_bMovingAllActors) {
				int currPos[2];
				this->Interactor->GetEventPosition(currPos);

				double new_pick_point[4], old_pick_point[4];
				this->ComputeDisplayToWorld(currPos[0], currPos[1], 0, new_pick_point);
				this->ComputeDisplayToWorld(LastPos[0], LastPos[1], 0, old_pick_point);

				// Compute rotation angle based on mouse movement
				double dx = new_pick_point[0] - old_pick_point[0];
				double dy = new_pick_point[1] - old_pick_point[1];
				double angle = sqrt(dx * dx + dy * dy) * 100; // Scale factor for rotation

				double axis[3] = { -dy, dx, 0 }; // Rotation axis perpendicular to mouse movement

				for (size_t i = 0; i < m_pActors.size(); i++)
				{
					if (!translations[i])
					{
						translations[i] = vtkSmartPointer<vtkTransform>::New();
					}

					// Apply rotation
					translations[i]->Identity();
					translations[i]->RotateWXYZ(angle, axis[0], axis[1], axis[2]);

					this->Renderer->RemoveActor(m_pActors[i]);
					m_pActors[i]->SetUserTransform(translations[i]);
					this->Renderer->AddActor(m_pActors[i]);
				}

				this->Interactor->Render();
				LastPos[0] = currPos[0];
				LastPos[1] = currPos[1];
			}
			else {
				bool bMoving = false;
				for (auto bmove : m_Moving)
					bMoving |= bmove;

				if (bMoving)
				{
					int currPos[2];
					this->Interactor->GetEventPosition(currPos);
					double new_pick_point[4];
					double old_pick_point[4], motion_vector[3];
					this->ComputeDisplayToWorld(currPos[0], currPos[1], 0, new_pick_point);
					this->ComputeDisplayToWorld(LastPos[0], LastPos[1], 0, old_pick_point);

					int j;
					// Move along the correct axis
					for (int i = 0; i < 6; i++) {

						if ((this->m_pTarget == m_pActors[i]) && (this->m_Moving[i])) {
							if (!translations[i]) {
								translations[i] = vtkSmartPointer<vtkTransform>::New();
							}
							switch (i) {
							case 0:
								j = 2;
								motion_vector[j] = (new_pick_point[j] - old_pick_point[j]);
								if (motion_vector[j] > 0) {
									total_vector[i][j] += increamentXYZ;
								}
								else if (motion_vector[j] < 0) {
									total_vector[i][j] -= increamentXYZ;
								}
								if (total_vector[i][j] > 0)
									total_vector[i][j] = 0;
								else if (total_vector[i][j] < 2 * m_bounds[0])
									total_vector[i][j] = 2 * m_bounds[0];

								translations[i]->Identity();
								translations[i]->Translate(0, 0, total_vector[i][j]);
								break;
							case 1:
								j = 2;
								motion_vector[j] = (new_pick_point[j] - old_pick_point[j]);
								if (motion_vector[j] > 0) {
									total_vector[i][j] += increamentXYZ;
								}
								else if (motion_vector[j] < 0) {
									total_vector[i][j] -= increamentXYZ;
								}
								if (total_vector[i][j] > 2 * m_bounds[1])
									total_vector[i][j] = 2 * m_bounds[1];
								else if (total_vector[i][j] < 0)
									total_vector[i][j] = 0;
								translations[i]->Identity();
								translations[i]->Translate(0, 0, total_vector[i][j]);
								break;
							case 2:
								j = 0;
								motion_vector[j] = (new_pick_point[j] - old_pick_point[j]);
								if (motion_vector[j] > 0) {
									total_vector[i][j] += increamentXYZ;
								}
								else if (motion_vector[j] < 0) {
									total_vector[i][j] -= increamentXYZ;
								}
								if (total_vector[i][j] > 2 * m_bounds[1])
									total_vector[i][j] = 2 * m_bounds[1];
								else if (total_vector[i][j] < 0)
									total_vector[i][j] = 0;
								translations[i]->Identity();
								translations[i]->Translate(total_vector[i][j], 0, 0);
								break;
							case 3:
								j = 0;
								motion_vector[j] = (new_pick_point[j] - old_pick_point[j]);
								if (motion_vector[j] > 0) {
									total_vector[i][j] += increamentXYZ;
								}
								else if (motion_vector[j] < 0) {
									total_vector[i][j] -= increamentXYZ;
								}
								if (total_vector[i][j] > 0)
									total_vector[i][j] = 0;
								else if (total_vector[i][j] < 2 * m_bounds[0])
									total_vector[i][j] = 2 * m_bounds[0];
								translations[i]->Identity();
								translations[i]->Translate(total_vector[i][j], 0, 0);
								break;
							case 4:
								j = 1;
								motion_vector[j] = (new_pick_point[j] - old_pick_point[j]);
								if (motion_vector[j] > 0) {
									total_vector[i][j] += increamentXYZ;
								}
								else if (motion_vector[j] < 0) {
									total_vector[i][j] -= increamentXYZ;
								}
								if (total_vector[i][j] > 2 * m_bounds[1])
									total_vector[i][j] = 2 * m_bounds[1];
								else if (total_vector[i][j] < 0)
									total_vector[i][j] = 0;
								translations[i]->Identity();
								translations[i]->Translate(0, total_vector[i][j], 0);
								break;
							case 5:
								j = 1;
								motion_vector[j] = (new_pick_point[j] - old_pick_point[j]);
								if (motion_vector[j] > 0) {
									total_vector[i][j] += increamentXYZ;
								}
								else if (motion_vector[j] < 0) {
									total_vector[i][j] -= increamentXYZ;
								}
								if (total_vector[i][j] > 0)
									total_vector[i][j] = 0;
								else if (total_vector[i][j] < 2 * m_bounds[0])
									total_vector[i][j] = 2 * m_bounds[0];
								translations[i]->Identity();
								translations[i]->Translate(0, total_vector[i][j], 0);
								break;
							}
							this->Renderer->RemoveActor(this->m_pTarget);
							this->m_pTarget->SetUserTransform(translations[i]);
							this->Renderer->AddActor(this->m_pTarget);
						}
					}
					this->Interactor->Render();

					this->LastPos[0] = currPos[0];
					this->LastPos[1] = currPos[1];
				}
				else {
					vtkInteractorStyleTrackballActor::OnMouseMove();
				}
			}
		}
		else {
			this->CurrentStyle->OnMouseMove();
		}
	}

	virtual void OnLeftButtonUp() override
	{
		if (this->CurrentStyle == this->ActorStyle) {
			for (auto& bmove : m_Moving)
				bmove = false;
			vtkInteractorStyleTrackballActor::OnLeftButtonUp();
		}
		else
		{
			this->CurrentStyle->OnLeftButtonUp();
		}
	}

	virtual void OnRightButtonUp() override
	{
		if (this->CurrentStyle == this->ActorStyle) {
			m_bMovingAllActors = false;
			vtkInteractorStyleTrackballActor::OnRightButtonUp();
		}
		else
		{
			this->CurrentStyle->OnRightButtonUp();
		}
	}


	void SetPlanes(const std::vector<vtkActor*>& pActors)
	{
		int i = 0;
		if (m_pActors.size() != pActors.size())
			m_pActors.resize(pActors.size());

		for (auto actor : pActors) {
			m_pActors[i++] = actor;
		}
	}

	void SetBounds(double* pBound)
	{
		for (int i = 0; i < 6; i++) {
			m_bounds[i] = pBound[i];
		}
	}

	virtual void OnKeyPress() override
	{
		std::string key = this->GetInteractor()->GetKeySym();
		if (key == "c") // Press 'c' to switch mode
		{
			if (this->CurrentStyle == this->CameraStyle)
			{
				this->CurrentStyle = this->ActorStyle;
				std::cout << "Switched to Actor Mode" << std::endl;
			}
			else
			{
				this->CurrentStyle = this->CameraStyle;
				std::cout << "Switched to Camera Mode" << std::endl;
			}
			this->GetInteractor()->SetInteractorStyle(this->CurrentStyle);
		}
		this->CurrentStyle->OnKeyPress();
	}

private:
	vtkSmartPointer<vtkInteractorStyleTrackballActor> ActorStyle;
	vtkSmartPointer<vtkInteractorStyleTrackballCamera> CameraStyle;
	vtkSmartPointer<vtkInteractorStyle> CurrentStyle;
	vtkSmartPointer<vtkRenderer>	Renderer = nullptr;
	vtkActor*						m_pTarget = nullptr;
	std::array<vtkSmartPointer<vtkTransform>, NUMOFPLANES>	translations = { nullptr };
	std::vector<vtkActor*>			m_pActors;
	std::array<bool, NUMOFPLANES>	m_Moving = { false };
	bool							m_bMovingAllActors = false;
	double m_bounds[6]{ 0, 0, 0, 0, 0, 0 };
	int LastPos[2]{ 0, 0 };

};

vtkStandardNewMacro(vtkCustomInteractorStyle);



int test4(int argc, char* argv[]);
int main(int argc, char* argv[])
{
	int ret = test4(argc, argv);

	return ret;
}

static unsigned char bkg[4] = { 51, 77, 102, 255 };

int test4(int argc, char* argv[])
{
	vtkObject::GlobalWarningDisplayOff();

	vtkNew<vtkNamedColors> colors;
	colors->SetColor("BkgColor", bkg[0], bkg[1], bkg[2], bkg[2]);

	vtkNew<vtkRenderer> aRenderer;
	vtkNew<vtkRenderWindow> renWin;
	renWin->AddRenderer(aRenderer);
	renWin->SetWindowName("MedicalDemo3");

	vtkNew<vtkRenderWindowInteractor> iren;
	iren->SetRenderWindow(renWin);

	aRenderer->SetBackground(colors->GetColor3d("BkgColor").GetData());
	renWin->SetSize(640, 480);

	std::array<vtkNew<vtkPlaneSource>, NUMOFPLANES> planes;
	std::array<vtkNew<vtkPolyDataMapper>, NUMOFPLANES> polyDataMapperList;
	std::array<vtkSmartPointer<vtkActor>, NUMOFPLANES> ActorList;
	std::array<vtkNew<vtkVertexGlyphFilter>, NUMOFPLANES> vertexGlyphFilterList;

	double offset = 10;
	double halfLength = 50.0; // Since edge length is 100, half is 50
	double pBounds[6] = { -50, 50, -50, 50, -50, 50 };
	for (int i = 0; i < 6; i++) {
		planes[i]->SetXResolution(10);
		planes[i]->SetYResolution(10);

		switch (i) {
		case 0: // Front face (XY plane, Z = +50)
			planes[i]->SetOrigin(-halfLength + offset, -halfLength + offset, halfLength);
			planes[i]->SetPoint1(halfLength - offset, -halfLength+ offset, halfLength);
			planes[i]->SetPoint2(-halfLength+ offset, halfLength- offset, halfLength);
			break;
		case 1: // Back face (XY plane, Z = -50)
			planes[i]->SetOrigin(-halfLength+ offset, -halfLength+ offset, -halfLength);
			planes[i]->SetPoint1(halfLength- offset, -halfLength+ offset, -halfLength);
			planes[i]->SetPoint2(-halfLength+ offset, halfLength- offset, -halfLength);
			break;
		case 2: // Left face (YZ plane, X = -50)
			planes[i]->SetOrigin(-halfLength, -halfLength+ offset, -halfLength+ offset);
			planes[i]->SetPoint1(-halfLength, halfLength- offset, -halfLength+ offset);
			planes[i]->SetPoint2(-halfLength, -halfLength+ offset, halfLength- offset);
			break;
		case 3: // Right face (YZ plane, X = +50)
			planes[i]->SetOrigin(halfLength, -halfLength+ offset, -halfLength+ offset);
			planes[i]->SetPoint1(halfLength, halfLength- offset, -halfLength+ offset);
			planes[i]->SetPoint2(halfLength, -halfLength+ offset, halfLength- offset);
			break;
		case 4: // Bottom face (XZ plane, Y = -50)
			planes[i]->SetOrigin(-halfLength+ offset, -halfLength, -halfLength+ offset);
			planes[i]->SetPoint1(halfLength- offset, -halfLength, -halfLength+ offset);
			planes[i]->SetPoint2(-halfLength+ offset, -halfLength, halfLength- offset);
			break;
		case 5: // Top face (XZ plane, Y = +50)
			planes[i]->SetOrigin(-halfLength+ offset, halfLength, -halfLength+ offset);
			planes[i]->SetPoint1(halfLength- offset, halfLength, -halfLength+ offset);
			planes[i]->SetPoint2(-halfLength+ offset, halfLength, halfLength- offset);
			break;
		}
	}

	for (int i = 0; i < NUMOFPLANES; i++) {
		int res = std::max(planes[i]->GetXResolution(), planes[i]->GetYResolution());
		if (res > length)
			length = res;
	}

	for (int i = 0; i < NUMOFPLANES; i++) {
		planes[i]->Update();
	}

	std::array<double, 6> bounds;
	for (int i = 0; i < NUMOFPLANES; i++) {
		planes[i]->GetOutput()->GetBounds(bounds.data());
		for (int j = 0; j < 6; j++)
			std::cout << bounds[j] << " ";
		std::cout << std::endl;
	}

	for (int i = 0; i < NUMOFPLANES; i++) {
		polyDataMapperList[i]->SetInputConnection(planes[i]->GetOutputPort());
		ActorList[i] = vtkSmartPointer<vtkActor>::New();
		ActorList[i]->SetMapper(polyDataMapperList[i]);
		ActorList[i]->GetProperty()->SetOpacity(0.3);
		aRenderer->AddActor(ActorList[i]);
	}

	vtkNew<vtkCamera> aCamera;
	aCamera->SetViewUp(0, 0, -1);
	aCamera->SetPosition(0, -1, 0);
	aCamera->SetFocalPoint(0, 0, 0);
	aCamera->ComputeViewPlaneNormal();
	aCamera->Azimuth(30.0);
	aCamera->Elevation(30.0);

	aRenderer->SetActiveCamera(aCamera);
	renWin->Render();

	aRenderer->ResetCamera();
	aCamera->Dolly(1.5);
	aRenderer->ResetCameraClippingRange();

	vtkNew<vtkCustomInteractorStyle> style;
	std::vector<vtkActor*>  actors;
	for (auto smtActor : ActorList)
		actors.push_back(smtActor.Get());
	style->SetRenderer(aRenderer);
	style->SetBounds(pBounds);
	style->SetPlanes(actors);
	iren->SetInteractorStyle(style);

	// interact with data
	iren->Initialize();
	iren->Start();

	return EXIT_SUCCESS;
}