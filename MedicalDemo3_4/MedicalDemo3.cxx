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
const double	offset = 10;
const double    epsilon = 1e-16;

class vtkCustomInteractorStyleCamera;

class vtkCustomInteractorStyle : public vtkInteractorStyleTrackballActor
{
public:
	static vtkCustomInteractorStyle* New();
	vtkTypeMacro(vtkCustomInteractorStyle, vtkInteractorStyleTrackballActor);
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

			for (int i = 0; i < NUMOFPLANES; i++)
			{
				if (this->m_pTarget == m_pActors[i])
				{
					m_Moving[i] = true;
					std::cout << i << " has been selected " << std::endl;
				}
				else {
					m_Moving[i] = false;
				}
			}
		}
		else {
			//vtkInteractorStyleTrackballActor::OnLeftButtonDown();
		}
	}


	virtual void OnMouseMove() override
	{
		int i = 0;
		for (; i < NUMOFPLANES; i++) {
			if (m_Moving[i]) {
				//std::cout << " i = " << i << std::endl;
				break;
			}
		}

		if (i < NUMOFPLANES)
		{

			vtkCamera* camera = this->Renderer->GetActiveCamera();
			if (!camera) {
				std::cerr << "Camera is null" << std::endl;
				return;
			}

			this->Interactor->GetRenderWindow()->Render();

			double	new_pick_point[4] = { 0 };
			double	old_pick_point[4] = { 0 };
			double	motion_vector[3] = { 0 };
			int		currPos[2];

			this->Interactor->GetEventPosition(currPos);

			double  displayCurrPos[3] = { currPos[0], currPos[1], 0 };
			double  displayLastPos[3] = { LastPos[0], LastPos[1], 0 };
			this->Renderer->SetDisplayPoint(displayCurrPos);
			this->Renderer->DisplayToWorld();
			this->Renderer->GetWorldPoint(new_pick_point);

			this->Renderer->SetDisplayPoint(displayLastPos);
			this->Renderer->DisplayToWorld();
			this->Renderer->GetWorldPoint(old_pick_point);

			int j;
			if (new_pick_point[3] == 0.0) {
				std::cerr << "Invalid world coordinates! w component of new_pick_point is zero. " << std::endl;
				return;
			}
			else {
				for (int n = 0; n < 3; n++)
					new_pick_point[n] /= new_pick_point[3];
			}

			if (old_pick_point[3] == 0.0) {
				std::cerr << "Invalid world coordinates! w component of old_pick_point is zero. " << std::endl;
				return;
			}
			else {
				for (int n = 0; n < 3; n++)
					old_pick_point[n] /= old_pick_point[3];
			}
			for (int n = 0; n < 3; n++) {
				motion_vector[n] = new_pick_point[n] - old_pick_point[n];
			}

			double	origin[3] = { 0 };
			double  pt1[3] = { 0 };
			double  pt2[3] = { 0 };

			double*  pBounds = nullptr;
			bool flagOrigDown;
			bool flagPt1Down;
			bool flagPt2Down;
			bool flagOrigUp;
			bool flagPt1Up;
			bool flagPt2Up;
			bool flagDown;
			bool flagUp;

			if ((this->m_pTarget == m_pActors[i]) && (this->m_Moving[i])) {
				if (!translations[i]) {
					translations[i] = vtkSmartPointer<vtkTransform>::New();
				}
				switch (i) {
				case 0:
					j = 2;
					if (motion_vector[j] > 0) {
						total_vector[i][j] += increamentXYZ;
					}
					else if (motion_vector[j] < 0) {
						total_vector[i][j] -= increamentXYZ;
					}
					if (total_vector[i][j] > m_bounds[2 * j + 1] - m_bounds[2 * j])
						total_vector[i][j] = m_bounds[2 * j + 1] - m_bounds[2 * j];
					else if (total_vector[i][j] < 0)
						total_vector[i][j] = 0;
					translations[i]->Identity();
					translations[i]->Translate(0, 0, total_vector[i][j]);
					for (int n = 0; n < 6; n++) {
						if (n == 0 || n == 1) continue;
						m_pPlaneSources[n]->GetOrigin(origin);
						m_pPlaneSources[n]->GetPoint1(pt1);
						m_pPlaneSources[n]->GetPoint2(pt2);

						flagOrigUp = abs(origin[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagPt1Up = abs(pt1[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagPt2Up = abs(pt2[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagOrigDown = abs(origin[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagPt1Down = abs(pt1[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagPt2Down = abs(pt2[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagDown = flagOrigDown && flagPt1Down && flagPt2Down;
						flagUp = flagOrigUp && flagPt1Up && flagPt2Up;

						if (flagDown)
						{
							pt2[j] = m_bounds[2 * j] + total_vector[i][j];
							if (pt2[j] > m_bounds[2 * j + 1] - offset)
								pt2[j] = m_bounds[2 * j + 1] - offset;
							else if (pt2[j] < m_bounds[2 * j] + offset)
								pt2[j] = m_bounds[2 * j] + offset;
						}
						else if (flagUp) {
							pt2[j] = m_bounds[2 * j] + total_vector[i][j];
							if (pt2[j] > m_bounds[2 * j + 1] - offset)
								pt2[j] = m_bounds[2 * j + 1] - offset;
							else if (pt2[j] < m_bounds[2 * j] + offset)
								pt2[j] = m_bounds[2 * j] + offset;
						}
						else {
							if (!flagOrigDown) {
								origin[j] = m_bounds[2 * j] + total_vector[i][j];
								if (origin[j] > m_bounds[2 * j + 1] - offset)
									origin[j] = m_bounds[2 * j + 1] - offset;
								else if (origin[j] < m_bounds[2 * j] + offset)
									origin[j] = m_bounds[2 * j] + offset;
							}
							if (!flagPt1Down) {
								pt1[j] = m_bounds[2 * j] + total_vector[i][j];
								if (pt1[j] > m_bounds[2 * j + 1] - offset)
									pt1[j] = m_bounds[2 * j + 1] - offset;
								else if (pt1[j] < m_bounds[2 * j] + offset)
									pt1[j] = m_bounds[2 * j] + offset;
							}
							if (!flagPt2Down) {
								pt2[j] = m_bounds[2 * j] + total_vector[i][j];
								if (pt2[j] > m_bounds[2 * j + 1] - offset)
									pt2[j] = m_bounds[2 * j + 1] - offset;
								else if (pt2[j] < m_bounds[2 * j] + offset)
									pt2[j] = m_bounds[2 * j] + offset;
							}
						}
						m_pPlaneSources[n]->SetOrigin(origin);
						m_pPlaneSources[n]->SetPoint1(pt1);
						m_pPlaneSources[n]->SetPoint2(pt2);

						this->Renderer->RemoveActor(m_pActors[n]);
						vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast(m_pActors[n]->GetMapper());
						mapper->SetInputConnection(m_pPlaneSources[n]->GetOutputPort());
						mapper->Update();
						this->Renderer->AddActor(m_pActors[n]);
					}
					break;
				case 1:
					j = 2;
					if (motion_vector[j] > 0) {
						total_vector[i][j] += increamentXYZ;
					}
					else if (motion_vector[j] < 0) {
						total_vector[i][j] -= increamentXYZ;
					}
					if (total_vector[i][j] > 0)
						total_vector[i][j] = 0;
					else if (total_vector[i][j] < m_bounds[2 * j] - m_bounds[2 * j + 1])
						total_vector[i][j] = m_bounds[2 * j] - m_bounds[2 * j + 1];

					translations[i]->Identity();
					translations[i]->Translate(0, 0, total_vector[i][j]);

					for (int n = 0; n < 6; n++) {
						if (n == 0 || n == 1) continue;
						m_pPlaneSources[n]->GetOrigin(origin);
						m_pPlaneSources[n]->GetPoint1(pt1);
						m_pPlaneSources[n]->GetPoint2(pt2);

						flagOrigDown = abs(origin[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagPt1Down = abs(pt1[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagPt2Down = abs(pt2[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagOrigUp = abs(origin[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagPt1Up = abs(pt1[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagPt2Up = abs(pt2[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagDown = flagOrigDown && flagPt1Down && flagPt2Down;
						flagUp = flagOrigUp && flagPt1Up && flagPt2Up;

						if (flagDown)
						{
							pt2[j] = m_bounds[2 * j + 1] + total_vector[i][j];
							if (pt2[j] > m_bounds[2 * j + 1] - offset)
								pt2[j] = m_bounds[2 * j + 1] - offset;
							else if (pt2[j] < m_bounds[2 * j] + offset)
								pt2[j] = m_bounds[2 * j] + offset;
						}
						else if (flagUp) {
							pt2[j] = m_bounds[2 * j + 1] + total_vector[i][j];
							if (pt2[j] > m_bounds[2 * j + 1] - offset)
								pt2[j] = m_bounds[2 * j + 1] - offset;
							else if (pt2[j] < m_bounds[2 * j] + offset)
								pt2[j] = m_bounds[2 * j] + offset;
						}
						else {
							if (!flagOrigDown) {
								origin[j] = m_bounds[2 * j + 1] + total_vector[i][j];
								if (origin[j] > m_bounds[2 * j + 1] - offset)
									origin[j] = m_bounds[2 * j + 1] - offset;
								else if (origin[j] < m_bounds[2 * j] + offset)
									origin[j] = m_bounds[2 * j] + offset;
							}
							if (!flagPt1Down) {
								pt1[j] = m_bounds[2 * j + 1] + total_vector[i][j];
								if (pt1[j] > m_bounds[2 * j + 1] - offset)
									pt1[j] = m_bounds[2 * j + 1] - offset;
								else if (pt1[j] < m_bounds[2 * j] + offset)
									pt1[j] = m_bounds[2 * j] + offset;
							}
							if (!flagPt2Down) {
								pt2[j] = m_bounds[2 * j + 1] + total_vector[i][j];
								if (pt2[j] > m_bounds[2 * j + 1] - offset)
									pt2[j] = m_bounds[2 * j + 1] - offset;
								else if (pt2[j] < m_bounds[2 * j] + offset)
									pt2[j] = m_bounds[2 * j] + offset;
							}
						}
						m_pPlaneSources[n]->SetOrigin(origin);
						m_pPlaneSources[n]->SetPoint1(pt1);
						m_pPlaneSources[n]->SetPoint2(pt2);

						this->Renderer->RemoveActor(m_pActors[n]);
						vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast(m_pActors[n]->GetMapper());
						mapper->SetInputConnection(m_pPlaneSources[n]->GetOutputPort());
						mapper->Update();
						this->Renderer->AddActor(m_pActors[n]);
					}
					break;
				case 2:
					j = 0;
					if (motion_vector[j] > 0) {
						total_vector[i][j] += increamentXYZ;
					}
					else if (motion_vector[j] < 0) {
						total_vector[i][j] -= increamentXYZ;
					}
					if (total_vector[i][j] > m_bounds[2 * j + 1] - m_bounds[2 * j])
						total_vector[i][j] = m_bounds[2 * j + 1] - m_bounds[2 * j];
					else if (total_vector[i][j] < 0)
						total_vector[i][j] = 0;
					translations[i]->Identity();
					translations[i]->Translate(total_vector[i][j], 0, 0);
					for (int n = 0; n < 6; n++) {
						if (n == 2 || n == 3) continue;
						m_pPlaneSources[n]->GetOrigin(origin);
						m_pPlaneSources[n]->GetPoint1(pt1);
						m_pPlaneSources[n]->GetPoint2(pt2);

						flagOrigUp = abs(origin[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagPt1Up = abs(pt1[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagPt2Up = abs(pt2[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagOrigDown = abs(origin[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagPt1Down = abs(pt1[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagPt2Down = abs(pt2[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagDown = flagOrigDown && flagPt1Down && flagPt2Down;
						flagUp = flagOrigUp && flagPt1Up && flagPt2Up;
						if (flagDown)
						{
							pt1[j] = m_bounds[2 * j] + total_vector[i][j];
							if (pt1[j] > m_bounds[2 * j + 1] - offset)
								pt1[j] = m_bounds[2 * j + 1] - offset;
							else if (pt1[j] < m_bounds[2 * j] + offset)
								pt1[j] = m_bounds[2 * j] + offset;
						}
						else if (flagUp) {
							pt1[j] = m_bounds[2 * j] + total_vector[i][j];
							if (pt1[j] > m_bounds[2 * j + 1] - offset)
								pt1[j] = m_bounds[2 * j + 1] - offset;
							else if (pt1[j] < m_bounds[2 * j] + offset)
								pt1[j] = m_bounds[2 * j] + offset;
						}
						else {
							if (!flagOrigDown) {
								origin[j] = m_bounds[2 * j] + total_vector[i][j];
								if (origin[j] > m_bounds[2 * j + 1] - offset)
									origin[j] = m_bounds[2 * j + 1] - offset;
								else if (origin[j] < m_bounds[2 * j] + offset)
									origin[j] = m_bounds[2 * j] + offset;
							}
							if (!flagPt1Down) {
								pt1[j] = m_bounds[2 * j] + total_vector[i][j];
								if (pt1[j] > m_bounds[2 * j + 1] - offset)
									pt1[j] = m_bounds[2 * j + 1] - offset;
								else if (pt1[j] < m_bounds[2 * j] + offset)
									pt1[j] = m_bounds[2 * j] + offset;
							}
							if (!flagPt2Down) {
								pt2[j] = m_bounds[2 * j] + total_vector[i][j];
								if (pt2[j] > m_bounds[2 * j + 1] - offset)
									pt2[j] = m_bounds[2 * j + 1] - offset;
								else if (pt2[j] < m_bounds[2 * j] + offset)
									pt2[j] = m_bounds[2 * j] + offset;
							}
						}
						m_pPlaneSources[n]->SetOrigin(origin);
						m_pPlaneSources[n]->SetPoint1(pt1);
						m_pPlaneSources[n]->SetPoint2(pt2);

						this->Renderer->RemoveActor(m_pActors[n]);
						vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast(m_pActors[n]->GetMapper());
						mapper->SetInputConnection(m_pPlaneSources[n]->GetOutputPort());
						mapper->Update();
						this->Renderer->AddActor(m_pActors[n]);
					}
					break;
				case 3:
					j = 0;
					if (motion_vector[j] > 0) {
						total_vector[i][j] += increamentXYZ;
					}
					else if (motion_vector[j] < 0) {
						total_vector[i][j] -= increamentXYZ;
					}
					if (total_vector[i][j] > 0)
						total_vector[i][j] = 0;
					else if (total_vector[i][j] < m_bounds[2 * j] - m_bounds[2 * j + 1])
						total_vector[i][j] = m_bounds[2 * j] - m_bounds[2 * j + 1];
					translations[i]->Identity();
					translations[i]->Translate(total_vector[i][j], 0, 0);
					for (int n = 0; n < 6; n++) {
						if (n == 2 || n == 3) continue;
						m_pPlaneSources[n]->GetOrigin(origin);
						m_pPlaneSources[n]->GetPoint1(pt1);
						m_pPlaneSources[n]->GetPoint2(pt2);

						flagOrigDown = abs(origin[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagPt1Down = abs(pt1[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagPt2Down = abs(pt2[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagOrigUp = abs(origin[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagPt1Up = abs(pt1[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagPt2Up = abs(pt2[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagDown = flagOrigDown && flagPt1Down && flagPt2Down;
						flagUp = flagOrigUp && flagPt1Up && flagPt2Up;

						if (flagDown)
						{
							pt1[j] = m_bounds[2 * j + 1] + total_vector[i][j];
							if (pt1[j] > m_bounds[2 * j + 1] - offset)
								pt1[j] = m_bounds[2 * j + 1] - offset;
							else if (pt1[j] < m_bounds[2 * j] + offset)
								pt1[j] = m_bounds[2 * j] + offset;
						}
						else if (flagUp) {
							pt1[j] = m_bounds[2 * j + 1] + total_vector[i][j];
							if (pt1[j] > m_bounds[2 * j + 1] - offset)
								pt1[j] = m_bounds[2 * j + 1] - offset;
							else if (pt1[j] < m_bounds[2 * j] + offset)
								pt1[j] = m_bounds[2 * j] + offset;
						}
						else {
							if (!flagOrigDown) {
								origin[j] = m_bounds[2 * j + 1] + total_vector[i][j];
								if (origin[j] > m_bounds[2 * j + 1] - offset)
									origin[j] = m_bounds[2 * j + 1] - offset;
								else if (origin[j] < m_bounds[2 * j] + offset)
									origin[j] = m_bounds[2 * j] + offset;
							}
							if (!flagPt1Down) {
								pt1[j] = m_bounds[2 * j + 1] + total_vector[i][j];
								if (pt1[j] > m_bounds[2 * j + 1] - offset)
									pt1[j] = m_bounds[2 * j + 1] - offset;
								else if (pt1[j] < m_bounds[2 * j] + offset)
									pt1[j] = m_bounds[2 * j] + offset;
							}
							if (!flagPt2Down) {
								pt2[j] = m_bounds[2 * j + 1] + total_vector[i][j];
								if (pt2[j] > m_bounds[2 * j + 1] - offset)
									pt2[j] = m_bounds[2 * j + 1] - offset;
								else if (pt2[j] < m_bounds[2 * j] + offset)
									pt2[j] = m_bounds[2 * j] + offset;
							}
						}
						m_pPlaneSources[n]->SetOrigin(origin);
						m_pPlaneSources[n]->SetPoint1(pt1);
						m_pPlaneSources[n]->SetPoint2(pt2);

						this->Renderer->RemoveActor(m_pActors[n]);
						vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast(m_pActors[n]->GetMapper());
						mapper->SetInputConnection(m_pPlaneSources[n]->GetOutputPort());
						mapper->Update();
						this->Renderer->AddActor(m_pActors[n]);
					}
					break;
				case 4:
					j = 1;
					if (motion_vector[j] > 0) {
						total_vector[i][j] += increamentXYZ;
					}
					else if (motion_vector[j] < 0) {
						total_vector[i][j] -= increamentXYZ;
					}
					if (total_vector[i][j] > m_bounds[2 * j + 1] - m_bounds[2 * j])
						total_vector[i][j] = m_bounds[2 * j + 1] - m_bounds[2 * j];
					else if (total_vector[i][j] < 0)
						total_vector[i][j] = 0;
					translations[i]->Identity();
					translations[i]->Translate(0, total_vector[i][j], 0);
					for (int n = 0; n < 6; n++) {
						if (n == 4 || n == 5) continue;
						m_pPlaneSources[n]->GetOrigin(origin);
						m_pPlaneSources[n]->GetPoint1(pt1);
						m_pPlaneSources[n]->GetPoint2(pt2);

						flagOrigUp = abs(origin[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagPt1Up = abs(pt1[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagPt2Up = abs(pt2[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagOrigDown = abs(origin[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagPt1Down = abs(pt1[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagPt2Down = abs(pt2[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;

						flagDown = flagOrigDown && flagPt1Down && flagPt2Down;
						flagUp = flagOrigUp && flagPt1Up && flagPt2Up;

						if (flagDown)
						{
							if (n == 2 || n == 3) {
								pt1[j] = m_bounds[2 * j] + total_vector[i][j];
								if (pt1[j] > m_bounds[2 * j + 1] - offset)
									pt1[j] = m_bounds[2 * j + 1] - offset;
								else if (pt1[j] < m_bounds[2 * j] + offset)
									pt1[j] = m_bounds[2 * j] + offset;
							}
							else {
								pt2[j] = m_bounds[2 * j] + total_vector[i][j];
								if (pt2[j] > m_bounds[2 * j + 1] - offset)
									pt2[j] = m_bounds[2 * j + 1] - offset;
								else if (pt2[j] < m_bounds[2 * j] + offset)
									pt2[j] = m_bounds[2 * j] + offset;
							}
						}
						else if (flagUp) {
							if (n == 2 || n == 3) {
								pt1[j] = m_bounds[2 * j] + total_vector[i][j];
								if (pt1[j] > m_bounds[2 * j + 1] - offset)
									pt1[j] = m_bounds[2 * j + 1] - offset;
								else if (pt1[j] < m_bounds[2 * j] + offset)
									pt1[j] = m_bounds[2 * j] + offset;
							}
							else {
								pt2[j] = m_bounds[2 * j] + total_vector[i][j];
								if (pt2[j] > m_bounds[2 * j + 1] - offset)
									pt2[j] = m_bounds[2 * j + 1] - offset;
								else if (pt2[j] < m_bounds[2 * j] + offset)
									pt2[j] = m_bounds[2 * j] + offset;
							}
						}
						else {
							if (!flagOrigDown) {
								origin[j] = m_bounds[2 * j] + total_vector[i][j];
								if (origin[j] > m_bounds[2 * j + 1] - offset)
									origin[j] = m_bounds[2 * j + 1] - offset;
								else if (origin[j] < m_bounds[2 * j] + offset)
									origin[j] = m_bounds[2 * j] + offset;
							}
							if (!flagPt1Down) {
								pt1[j] = m_bounds[2 * j] + total_vector[i][j];
								if (pt1[j] > m_bounds[2 * j + 1] - offset)
									pt1[j] = m_bounds[2 * j + 1] - offset;
								else if (pt1[j] < m_bounds[2 * j] + offset)
									pt1[j] = m_bounds[2 * j] + offset;
							}
							if (!flagPt2Down) {
								pt2[j] = m_bounds[2 * j] + total_vector[i][j];
								if (pt2[j] > m_bounds[2 * j + 1] - offset)
									pt2[j] = m_bounds[2 * j + 1] - offset;
								else if (pt2[j] < m_bounds[2 * j] + offset)
									pt2[j] = m_bounds[2 * j] + offset;
							}
						}
						m_pPlaneSources[n]->SetOrigin(origin);
						m_pPlaneSources[n]->SetPoint1(pt1);
						m_pPlaneSources[n]->SetPoint2(pt2);

						this->Renderer->RemoveActor(m_pActors[n]);
						vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast(m_pActors[n]->GetMapper());
						mapper->SetInputConnection(m_pPlaneSources[n]->GetOutputPort());
						mapper->Update();
						this->Renderer->AddActor(m_pActors[n]);
					}
					break;
				case 5:
					j = 1;
					if (motion_vector[j] > 0) {
						total_vector[i][j] += increamentXYZ;
					}
					else if (motion_vector[j] < 0) {
						total_vector[i][j] -= increamentXYZ;
					}
					if (total_vector[i][j] > 0)
						total_vector[i][j] = 0;
					else if (total_vector[i][j] < m_bounds[2 * j] - m_bounds[2 * j + 1])
						total_vector[i][j] = m_bounds[2 * j] - m_bounds[2 * j + 1];
					translations[i]->Identity();
					translations[i]->Translate(0, total_vector[i][j], 0);
					for (int n = 0; n < 6; n++) {
						if (n == 4 || n == 5) continue;
						m_pPlaneSources[n]->GetOrigin(origin);
						m_pPlaneSources[n]->GetPoint1(pt1);
						m_pPlaneSources[n]->GetPoint2(pt2);

						flagOrigDown = abs(origin[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagPt1Down = abs(pt1[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagPt2Down = abs(pt2[j] - (m_bounds[2 * j] + offset)) < epsilon;
						flagOrigUp = abs(origin[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagPt1Up = abs(pt1[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagPt2Up = abs(pt2[j] - (m_bounds[2 * j + 1] - offset)) < epsilon;
						flagDown = flagOrigDown && flagPt1Down && flagPt2Down;
						flagUp = flagOrigUp && flagPt1Up && flagPt2Up;

						if (flagDown)
						{
							if (n == 2 || n == 3) {
								pt1[j] = m_bounds[2 * j + 1] + total_vector[i][j];
								if (pt1[j] > m_bounds[2 * j + 1] - offset)
									pt1[j] = m_bounds[2 * j + 1] - offset;
								else if (pt1[j] < m_bounds[2 * j] + offset)
									pt1[j] = m_bounds[2 * j] + offset;
							}
							else {
								pt2[j] = m_bounds[2 * j + 1] + total_vector[i][j];
								if (pt2[j] > m_bounds[2 * j + 1] - offset)
									pt2[j] = m_bounds[2 * j + 1] - offset;
								else if (pt2[j] < m_bounds[2 * j] + offset)
									pt2[j] = m_bounds[2 * j] + offset;
							}
						}
						else if (flagUp) {
							if (n == 2 || n == 3) {
								pt1[j] = m_bounds[2 * j + 1] + total_vector[i][j];
								if (pt1[j] > m_bounds[2 * j + 1] - offset)
									pt1[j] = m_bounds[2 * j + 1] - offset;
								else if (pt1[j] < m_bounds[2 * j] + offset)
									pt1[j] = m_bounds[2 * j] + offset;
							}
							else {
								pt2[j] = m_bounds[2 * j + 1] + total_vector[i][j];
								if (pt2[j] > m_bounds[2 * j + 1] - offset)
									pt2[j] = m_bounds[2 * j + 1] - offset;
								else if (pt2[j] < m_bounds[2 * j] + offset)
									pt2[j] = m_bounds[2 * j] + offset;
							}
						}
						else {
							if (!flagOrigDown) {
								origin[j] = m_bounds[2 * j + 1] + total_vector[i][j];
								if (origin[j] > m_bounds[2 * j + 1] - offset)
									origin[j] = m_bounds[2 * j + 1] - offset;
								else if (origin[j] < m_bounds[2 * j] + offset)
									origin[j] = m_bounds[2 * j] + offset;
							}
							if (!flagPt1Down) {
								pt1[j] = m_bounds[2 * j + 1] + total_vector[i][j];
								if (pt1[j] > m_bounds[2 * j + 1] - offset)
									pt1[j] = m_bounds[2 * j + 1] - offset;
								else if (pt1[j] < m_bounds[2 * j] + offset)
									pt1[j] = m_bounds[2 * j] + offset;
							}
							if (!flagPt2Down) {
								pt2[j] = m_bounds[2 * j + 1] + total_vector[i][j];
								if (pt2[j] > m_bounds[2 * j + 1] - offset)
									pt2[j] = m_bounds[2 * j + 1] - offset;
								else if (pt2[j] < m_bounds[2 * j] + offset)
									pt2[j] = m_bounds[2 * j] + offset;
							}
						}
						m_pPlaneSources[n]->SetOrigin(origin);
						m_pPlaneSources[n]->SetPoint1(pt1);
						m_pPlaneSources[n]->SetPoint2(pt2);

						this->Renderer->RemoveActor(m_pActors[n]);
						vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast(m_pActors[n]->GetMapper());
						mapper->SetInputConnection(m_pPlaneSources[n]->GetOutputPort());
						mapper->Update();
						this->Renderer->AddActor(m_pActors[n]);
					}
					break;
				}
				this->Renderer->RemoveActor(this->m_pTarget);
				this->m_pTarget->SetUserTransform(translations[i]);
				this->Renderer->AddActor(this->m_pTarget);

				//std::cout << i << " is moving " << std::endl;
			}

			//this->m_pTarget->SetPosition(pos);
			this->Interactor->Render();

			this->LastPos[0] = currPos[0];
			this->LastPos[1] = currPos[1];
			//this->Interactor->GetLastEventPosition(this->LastPos);
		}
		else {
			//vtkInteractorStyleTrackballActor::OnMouseMove();
		}
	}

	virtual void OnLeftButtonUp() override
	{
		for (auto& bmove : m_Moving)
			bmove = false;
		//vtkInteractorStyleTrackballActor::OnLeftButtonUp();
	}

	void SetRenderer(vtkRenderer* renderer) {
		this->Renderer = renderer;
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

	void SetPlaneSource(const std::vector<vtkPlaneSource*> pPlaneSource) {
		int i = 0;
		if (m_pPlaneSources.size() != pPlaneSource.size())
			m_pPlaneSources.resize(pPlaneSource.size());

		for (auto planeSource : pPlaneSource) {
			m_pPlaneSources[i++] = planeSource;
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
			this->CurrentStyle = this->CameraStyle;
			std::cout << "Switched to Camera Mode" << std::endl;

			this->GetInteractor()->SetInteractorStyle(this->CurrentStyle);
		}
		//this->CurrentStyle->OnKeyPress();
	}

	void SetStyle(vtkSmartPointer<vtkCustomInteractorStyleCamera> pCameraStyle,
		vtkSmartPointer<vtkInteractorStyle> pCurrentStyle)
	{
		this->CameraStyle = pCameraStyle;
		this->CurrentStyle = pCurrentStyle;
	}

private:
	vtkSmartPointer<vtkRenderer>	Renderer = nullptr;
	vtkActor*						m_pTarget = nullptr;
	std::array<vtkSmartPointer<vtkTransform>, NUMOFPLANES>	translations = { nullptr };
	std::vector<vtkActor*>			m_pActors;
	std::vector<vtkPlaneSource*>	m_pPlaneSources;
	std::array<bool, NUMOFPLANES>	m_Moving = { false };
	double							m_bounds[6]{ 0, 0, 0, 0, 0, 0 };
	int								LastPos[2]{ 0, 0 };
	vtkSmartPointer<vtkCustomInteractorStyleCamera> CameraStyle;
	vtkSmartPointer<vtkInteractorStyle> CurrentStyle;

};

vtkStandardNewMacro(vtkCustomInteractorStyle);

class vtkCustomInteractorStyleCamera : public vtkInteractorStyleTrackballCamera
{
public:
	static vtkCustomInteractorStyleCamera* New();
	vtkTypeMacro(vtkCustomInteractorStyleCamera, vtkInteractorStyleTrackballCamera);


	vtkCustomInteractorStyleCamera()
	{
		this->ActorStyle = vtkSmartPointer<vtkCustomInteractorStyle>::New();
		this->CurrentStyle = vtkSmartPointer<vtkInteractorStyle>::New();
		this->ActorStyle->SetStyle(this, CurrentStyle);
	}

	virtual void OnKeyPress() override
	{
		std::string key = this->GetInteractor()->GetKeySym();
		if (key == "c") // Press 'c' to switch mode
		{
			this->CurrentStyle = this->ActorStyle;
			std::cout << "Switched to Actor Mode" << std::endl;
			this->GetInteractor()->SetInteractorStyle(this->CurrentStyle);
		}
		//this->CurrentStyle->OnKeyPress();
	}

	void SetRenderer(vtkRenderer* renderer) { this->ActorStyle->SetRenderer(renderer); }
	void SetPlanes(const std::vector<vtkActor*>& pActors)
	{
		this->ActorStyle->SetPlanes(pActors);
	}
	void SetPlaneSource(const std::vector<vtkPlaneSource*> pPlaneSource) {
		this->ActorStyle->SetPlaneSource(pPlaneSource);
	}

	void SetBounds(double* pBound)
	{
		this->ActorStyle->SetBounds(pBound);
	}

	vtkSmartPointer<vtkCustomInteractorStyle> ActorStyle;
	vtkSmartPointer<vtkInteractorStyle> CurrentStyle;
};

vtkStandardNewMacro(vtkCustomInteractorStyleCamera);

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


	double halfLength = 50.0; // Since edge length is 100, half is 50
	double pBounds[6] = { -50, 50, -50, 50, -50, 50 };
	for (int i = 0; i < 6; i++) {
		planes[i]->SetXResolution(10);
		planes[i]->SetYResolution(10);

		switch (i) {
		case 0: // Back face (XY plane, Z = -50)
			planes[i]->SetOrigin(-halfLength + offset, -halfLength + offset, -halfLength);
			planes[i]->SetPoint1(halfLength - offset, -halfLength + offset, -halfLength);
			planes[i]->SetPoint2(-halfLength + offset, halfLength - offset, -halfLength);
			break;
		case 1: // Front face (XY plane, Z = +50)
			planes[i]->SetOrigin(-halfLength + offset, -halfLength + offset, halfLength);
			planes[i]->SetPoint1(halfLength - offset, -halfLength + offset, halfLength);
			planes[i]->SetPoint2(-halfLength + offset, halfLength - offset, halfLength);
			break;
		case 2: // Left face (YZ plane, X = -50)
			planes[i]->SetOrigin(-halfLength, -halfLength + offset, -halfLength + offset);
			planes[i]->SetPoint1(-halfLength, halfLength - offset, -halfLength + offset);
			planes[i]->SetPoint2(-halfLength, -halfLength + offset, halfLength - offset);
			break;
		case 3: // Right face (YZ plane, X = +50)
			planes[i]->SetOrigin(halfLength, -halfLength + offset, -halfLength + offset);
			planes[i]->SetPoint1(halfLength, halfLength - offset, -halfLength + offset);
			planes[i]->SetPoint2(halfLength, -halfLength + offset, halfLength - offset);
			break;
		case 4: // Bottom face (XZ plane, Y = -50)
			planes[i]->SetOrigin(-halfLength + offset, -halfLength, -halfLength + offset);
			planes[i]->SetPoint1(halfLength - offset, -halfLength, -halfLength + offset);
			planes[i]->SetPoint2(-halfLength + offset, -halfLength, halfLength - offset);
			break;
		case 5: // Top face (XZ plane, Y = +50)
			planes[i]->SetOrigin(-halfLength + offset, halfLength, -halfLength + offset);
			planes[i]->SetPoint1(halfLength - offset, halfLength, -halfLength + offset);
			planes[i]->SetPoint2(-halfLength + offset, halfLength, halfLength - offset);
			break;
		}
	}

	for (int i = 0; i < NUMOFPLANES; i++) {
		int res = std::max(planes[i]->GetXResolution(), planes[i]->GetYResolution());
		if (res > length)
			length = res;
	}

	//std::cout << "length = " << length << std::endl;

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

	vtkNew<vtkCustomInteractorStyleCamera> style;
	std::vector<vtkActor*>			actors(ActorList.begin(), ActorList.end());
	std::vector<vtkPlaneSource*>	planeSources(planes.begin(), planes.end());
	style->SetRenderer(aRenderer);
	style->SetBounds(pBounds);
	style->SetPlanes(actors);
	style->SetPlaneSource(planeSources);
	iren->SetInteractorStyle(style);

	// interact with data
	iren->Initialize();
	iren->Start();

	return EXIT_SUCCESS;
}