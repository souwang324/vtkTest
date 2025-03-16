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


static int length = 0;
static double total_vector[3] = { 0.0, 0.0, 0.0 };
static double pColor[3] = { 0.0, 0.0, 0.0 };
const double increamentXYZ = 0.005;

class vtkCustomInteractorStyle : public vtkInteractorStyleTrackballActor
{
public:
	static vtkCustomInteractorStyle* New();
	vtkTypeMacro(vtkCustomInteractorStyle, vtkInteractorStyleTrackballActor);

	void SetRenderer(vtkRenderer* renderer) { this->Renderer = renderer; }

	virtual void OnLeftButtonDown() override
	{
		int clickPos[2];
		this->Interactor->GetEventPosition(clickPos);
		vtkSmartPointer<vtkPropPicker> picker = vtkSmartPointer<vtkPropPicker>::New();
		picker->Pick(clickPos[0], clickPos[1], 0, this->Renderer);
		this->m_pTarget = vtkActor::SafeDownCast(picker->GetActor());

		if (this->m_pTarget)
		{
			this->Interactor->GetEventPosition(this->LastPos);

			if (this->m_pTarget == m_pActorX) { // Move along X-axis	
				this->MovingX = true;
				this->MovingY = false;
				this->MovingZ = false;
				if (!m_bInitX) {
					m_pActorX->GetBounds(boundsX);
					m_bInitX = true;
				}
			}
			else if (this->m_pTarget == m_pActorY) {// Move along Y-axis	
				this->MovingY = true;
				this->MovingX = false;
				this->MovingZ = false;
				if (!m_bInitY) {
					m_pActorY->GetBounds(boundsY);
					m_bInitY = true;
				}
			}
			else if (this->m_pTarget == m_pActorZ) {// Move along Z-axis	
				this->MovingZ = true;
				this->MovingX = false;
				this->MovingY = false;
				if (!m_bInitZ) {
					m_pActorZ->GetBounds(boundsZ);
					m_bInitZ = true;
				}
			}

			m_bInit = m_bInitX && m_bInitY && m_bInitZ;

			if (m_bInit)
			{
				for (int i = 0; i < 6; i += 2) {
					std::vector<double> v0 = { boundsX[i],boundsY[i],boundsZ[i] };
					bounds[i] = *std::min_element(v0.begin(), v0.end());
					std::vector<double> v1 = { boundsX[i+1],boundsY[i+1],boundsZ[i+1] };
					bounds[i+1] = *std::max_element(v1.begin(), v1.end());
				}
				std::cout << "bounds : ";
				for (int i = 0; i < 6; i++)
				{
					std::cout << bounds[i] << " ";
				}
				std::cout << std::endl;
			}
		}

		//vtkInteractorStyleTrackballActor::OnLeftButtonDown();
	}

	virtual void OnMouseMove() override
	{
		if (!m_bInit) return;

		if (((this->MovingX)|| (this->MovingY)|| (this->MovingZ)))
		{
			int currPos[2];
			this->Interactor->GetRenderWindow()->Render();
			this->Interactor->GetEventPosition(currPos);
			double displayCoordCurrPos[3] = { currPos[0], currPos[1], 0 };
			double displayCoordLastPos[3] = { LastPos[0], LastPos[1], 0 };
			double new_pick_point[4];
			double old_pick_point[4], motion_vector[3];


			this->Renderer->SetDisplayPoint(displayCoordCurrPos);
			this->Renderer->DisplayToWorld();
			this->Renderer->GetWorldPoint(new_pick_point);

			this->Renderer->SetDisplayPoint(displayCoordLastPos);
			this->Renderer->DisplayToWorld();
			this->Renderer->GetWorldPoint(old_pick_point);

			// Move along the correct axis
			if ((this->m_pTarget == m_pActorY) && (this->MovingY)) { // Move along Y-axis	
				if (!translationY) {
					translationY = vtkSmartPointer<vtkTransform>::New();						
				}
				int i = 1;
				motion_vector[i] = (new_pick_point[i] - old_pick_point[i]);
				if (motion_vector[i] > 0) {
					total_vector[i] += increamentXYZ;
				}
				else if (motion_vector[i] < 0) {
					total_vector[i] -= increamentXYZ;
				}
				if (total_vector[i] > bounds[2 * i + 1])
					total_vector[i] = bounds[2 * i + 1];
				else if (total_vector[i] < bounds[2 * i])
					total_vector[i] = bounds[2 * i];
				translationY->Identity();
				translationY->Translate(0, total_vector[1], 0);
				this->Renderer->RemoveActor(this->m_pTarget);
				this->m_pTarget->SetUserTransform(translationY);
				this->Renderer->AddActor(this->m_pTarget);
			}
			else if ((this->m_pTarget == m_pActorZ)&& (this->MovingZ)) { // Move along Z-axis
				if (!translationZ) {
					translationZ = vtkSmartPointer<vtkTransform>::New();
				}
				int i = 2;
				motion_vector[i] = (new_pick_point[i] - old_pick_point[i]);
				if (motion_vector[i] > 0) {
					total_vector[i] += increamentXYZ;
				}
				else if (motion_vector[i] < 0) {
					total_vector[i] -= increamentXYZ;
				}
				if (total_vector[i] > bounds[2 * i + 1])
					total_vector[i] = bounds[2 * i + 1];
				else if (total_vector[i] < bounds[2 * i])
					total_vector[i] = bounds[2 * i];
				translationZ->Identity();
				translationZ->Translate(0, 0, total_vector[2]);
					this->Renderer->RemoveActor(this->m_pTarget);
				this->m_pTarget->SetUserTransform(translationZ);
				this->Renderer->AddActor(this->m_pTarget);
			}
			else if ((this->m_pTarget == m_pActorX)&& (this->MovingX)) { // Move along X-axis
				if (!translationX) {
					translationX = vtkSmartPointer<vtkTransform>::New();
				}
				int i = 0;
				motion_vector[i] = (new_pick_point[i] - old_pick_point[i]);
				if (motion_vector[i] > 0) {
					total_vector[i] += increamentXYZ;
				}
				else if (motion_vector[i] < 0) {
					total_vector[i] -= increamentXYZ;
				}
				if (total_vector[i] > bounds[2 * i + 1])
					total_vector[i] = bounds[2 * i + 1];
				else if (total_vector[i] < bounds[2 * i])
					total_vector[i] = bounds[2 * i];
				translationX->Identity();
				translationX->Translate(total_vector[0], 0, 0);
				this->Renderer->RemoveActor(this->m_pTarget);
				this->m_pTarget->SetUserTransform(translationX);
				this->Renderer->AddActor(this->m_pTarget);
			}
			else
			{
				return;
			}
			this->Interactor->Render();

			this->LastPos[0] = currPos[0];
			this->LastPos[1] = currPos[1];
		}
	}

	virtual void OnLeftButtonUp() override
	{
		this->MovingX = false;
		this->MovingY = false;
		this->MovingZ = false;
		vtkInteractorStyleTrackballActor::OnLeftButtonUp();
	}

	void SetPlanes(vtkActor* pActorX, vtkActor* pActorY,vtkActor* pActorZ)
	{
		m_pActorX = pActorX;
		m_pActorY = pActorY;
		m_pActorZ = pActorZ;
	}

private:
	vtkSmartPointer<vtkRenderer> Renderer = nullptr;
	vtkSmartPointer<vtkTransform> translationX = nullptr;
	vtkSmartPointer<vtkTransform> translationY = nullptr;
	vtkSmartPointer<vtkTransform> translationZ = nullptr;
	vtkActor* m_pTarget = nullptr;
	vtkActor* m_pActorX = nullptr;
	vtkActor* m_pActorY = nullptr;
	vtkActor* m_pActorZ = nullptr;

	bool  m_bInitX = false;
	bool  m_bInitY = false;
	bool  m_bInitZ = false;
	bool  m_bInit = false;
	double boundsX[6]{ 0, 0, 0, 0, 0, 0};
	double boundsY[6]{ 0, 0, 0, 0, 0, 0 };
	double boundsZ[6]{ 0, 0, 0, 0, 0, 0 };
	double bounds[6]{ 0, 0, 0, 0, 0, 0 };
	int LastPos[2]{ 0, 0 };
	bool MovingX = false;
	bool MovingY = false;
	bool MovingZ = false;
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

	std::array<vtkNew<vtkPlaneSource>, 3> planes;
	std::array<vtkNew<vtkPolyDataMapper>, 3> polyDataMapperList;
	std::array<vtkSmartPointer<vtkActor>, 3> ActorList = { vtkSmartPointer<vtkActor>::New(),
														vtkSmartPointer<vtkActor>::New(),vtkSmartPointer<vtkActor>::New() };
	std::array<vtkNew<vtkVertexGlyphFilter>, 3> vertexGlyphFilterList;

	for (int i = 0; i < 3; i++) {
		planes[i]->SetCenter(0.0, 0.0, 0.0);
	}
	planes[0]->SetNormal(50, 0, 0);
	planes[1]->SetNormal(0, 50, 0);
	planes[2]->SetNormal(0, 0, 50);

	for (int i = 0; i < 3; i++) {
		int res = std::max(planes[i]->GetXResolution(), planes[i]->GetYResolution());
		if (res > length)
			length = res;
	}

	std::cout << "length = " << length << std::endl;

	for (int i = 0; i < 3; i++) {
		planes[i]->Update();
	}

	std::array<double, 6> bounds;
	for (int i = 0; i < 3; i++) {
		planes[i]->GetOutput()->GetBounds(bounds.data());
		for (int j = 0; j < 6; j++)
			std::cout << bounds[j] << " ";
		std::cout << std::endl;
	}

	for (int i = 0; i < 3; i++) {
		polyDataMapperList[i]->SetInputConnection(planes[i]->GetOutputPort());
		ActorList[i]->SetMapper(polyDataMapperList[i]);
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
	style->SetRenderer(aRenderer);
	style->SetPlanes(ActorList[0].Get(), ActorList[1].Get(), ActorList[2].Get());
	iren->SetInteractorStyle(style);

	// interact with data
	iren->Initialize();
	iren->Start();

	return EXIT_SUCCESS;
}