/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceCursorLineRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "ResliceCursorLineRepresentation.h"
#include "vtkActor2D.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkHandleRepresentation.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageReslice.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPlaneSource.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkResliceCursor.h"
#include "vtkResliceCursorActor.h"
#include "vtkResliceCursorPicker.h"
#include "vtkResliceCursorPolyDataAlgorithm.h"
#include "vtkSmartPointer.h"
#include "vtkTextActor.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkWindow.h"

#include <sstream>

vtkStandardNewMacro(ResliceCursorLineRepresentation);

//------------------------------------------------------------------------------
ResliceCursorLineRepresentation::ResliceCursorLineRepresentation()
{
  this->ResliceCursorActor = vtkResliceCursorActor::New();

  this->Picker = vtkResliceCursorPicker::New();
  this->Picker->SetTolerance(0.025);

  this->MatrixReslice = vtkMatrix4x4::New();
  this->MatrixView = vtkMatrix4x4::New();
  this->MatrixReslicedView = vtkMatrix4x4::New();
}

//------------------------------------------------------------------------------
ResliceCursorLineRepresentation::~ResliceCursorLineRepresentation()
{
  this->ResliceCursorActor->Delete();
  this->Picker->Delete();
  this->MatrixReslice->Delete();
  this->MatrixView->Delete();
  this->MatrixReslicedView->Delete();
}

//------------------------------------------------------------------------------
vtkResliceCursor* ResliceCursorLineRepresentation::GetResliceCursor()
{
  return this->ResliceCursorActor->GetCursorAlgorithm()->GetResliceCursor();
}

//------------------------------------------------------------------------------
int ResliceCursorLineRepresentation::ComputeInteractionState(int X, int Y, int modify)
{
  this->InteractionState = ResliceCursorLineRepresentation::Outside;

  if (!this->Renderer)
  {
    return this->InteractionState;
  }

  vtkResliceCursor* rc = this->GetResliceCursor();
  if (!rc)
  {
    vtkErrorMacro(<< "Reslice cursor not set!");
    return this->InteractionState;
  }

  this->Modifier = modify;

  // Ensure that the axis is initialized..
  const int axis1 = this->ResliceCursorActor->GetCursorAlgorithm()->GetAxis1();
  double bounds[6];
  this->ResliceCursorActor->GetCenterlineActor(axis1)->GetBounds(bounds);
  if (bounds[1] < bounds[0])
  {
    return this->InteractionState;
  }

  // Pick
  this->Picker->SetResliceCursorAlgorithm(this->ResliceCursorActor->GetCursorAlgorithm());
  int picked = this->Picker->Pick(X, Y, 0, this->Renderer);

  const bool pickedAxis1 = this->Picker->GetPickedAxis1() ? true : false;
  const bool pickedAxis2 = this->Picker->GetPickedAxis2() ? true : false;
  const bool pickedCenter = this->Picker->GetPickedCenter() ? true : false;
  if (picked)
  {
    this->Picker->GetPickPosition(this->StartPickPosition);
  }

  // Now assign the interaction state

  if (pickedCenter)
  {
    this->InteractionState = ResliceCursorLineRepresentation::OnCenter;
  }
  /*else if (pickedAxis1)
  {
    this->InteractionState = ResliceCursorLineRepresentation::OnAxis1;
  }
  else if (pickedAxis2)
  {
    this->InteractionState = ResliceCursorLineRepresentation::OnAxis2;
  }*/

  return this->InteractionState;
}

//------------------------------------------------------------------------------
// Record the current event position, and the center position.
void ResliceCursorLineRepresentation::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];

  if (this->ManipulationMode == WindowLevelling)
  {
    this->InitialWindow = this->CurrentWindow;
    this->InitialLevel = this->CurrentLevel;
  }
  else
  {
    if (vtkResliceCursor* rc = this->GetResliceCursor())
    {
      rc->GetCenter(this->StartCenterPosition);
    }
  }

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];
}

//------------------------------------------------------------------------------
void ResliceCursorLineRepresentation::WidgetInteraction(double e[2])
{
  vtkResliceCursor* rc = this->GetResliceCursor();

  if (this->ManipulationMode == WindowLevelling)
  {
    this->WindowLevel(e[0], e[1]);
    this->LastEventPosition[0] = e[0];
    this->LastEventPosition[1] = e[1];
    return;
  }

  // Depending on the state, different motions are allowed.

  if (this->InteractionState == Outside || !this->Renderer || !rc)
  {
    this->LastEventPosition[0] = e[0];
    this->LastEventPosition[1] = e[1];
    return;
  }

  if (rc->GetThickMode() &&
    this->ManipulationMode == vtkResliceCursorRepresentation::ResizeThickness)
  {

    double sf = 1.0;

    // Compute the scale factor
    const int* size = this->Renderer->GetSize();
    double dPos = e[1] - this->LastEventPosition[1];
    sf *= (1.0 + 2.0 * (dPos / size[1])); // scale factor of 2.0 is arbitrary

    double thickness[3];
    rc->GetThickness(thickness);
    rc->SetThickness(thickness[0] * sf, thickness[1] * sf, thickness[2] * sf);

    this->LastEventPosition[0] = e[0];
    this->LastEventPosition[1] = e[1];

    return;
  }

  // depending on the state, perform different operations
  //
  // 1. Translation

  if (this->InteractionState == OnCenter && !this->Modifier)
  {

    // Intersect with the viewing vector. We will use this point and the
    // start event point to compute an offset vector to translate the
    // center by.

    double intersectionPos[3], newCenter[3];
    this->Picker->Pick(e, intersectionPos, this->Renderer);

    // Offset the center by this vector.

    for (int i = 0; i < 3; i++)
    {
      newCenter[i] = this->StartCenterPosition[i] + intersectionPos[i] - this->StartPickPosition[i];
    }

    rc->SetCenter(newCenter);
  }

  // 2. Rotation of axis 1

  if (this->InteractionState == OnAxis1 && !this->Modifier)
  {
    this->RotateAxis(e, this->ResliceCursorActor->GetCursorAlgorithm()->GetPlaneAxis1());
  }

  // 3. Rotation of axis 2

  if (this->InteractionState == OnAxis2 && !this->Modifier)
  {
    this->RotateAxis(e, this->ResliceCursorActor->GetCursorAlgorithm()->GetPlaneAxis2());
  }

  // 4. Rotation of both axes

  if ((this->InteractionState == OnAxis2 || this->InteractionState == OnAxis1) && this->Modifier)
  {
    // Rotate both by the same angle
    const double angle =
      this->RotateAxis(e, this->ResliceCursorActor->GetCursorAlgorithm()->GetPlaneAxis1());
    this->RotateAxis(this->ResliceCursorActor->GetCursorAlgorithm()->GetPlaneAxis2(), angle);
  }

  this->LastEventPosition[0] = e[0];
  this->LastEventPosition[1] = e[1];
}

//------------------------------------------------------------------------------
double ResliceCursorLineRepresentation::RotateAxis(double e[2], int axis)
{
  vtkResliceCursor* rc = this->GetResliceCursor();

  double center[3];
  rc->GetCenter(center);

  // Intersect with the viewing vector. We will use this point and the
  // start event point to compute the rotation angle

  double currIntersectionPos[3], lastIntersectionPos[3];
  this->DisplayToReslicePlaneIntersection(e, currIntersectionPos);
  this->DisplayToReslicePlaneIntersection(this->LastEventPosition, lastIntersectionPos);

  if (lastIntersectionPos[0] == currIntersectionPos[0] &&
    lastIntersectionPos[1] == currIntersectionPos[1] &&
    lastIntersectionPos[2] == currIntersectionPos[2])
  {
    return 0;
  }

  double lastVector[3], currVector[3];
  for (int i = 0; i < 3; i++)
  {
    lastVector[i] = lastIntersectionPos[i] - center[i];
    currVector[i] = currIntersectionPos[i] - center[i];
  }

  vtkMath::Normalize(lastVector);
  vtkMath::Normalize(currVector);

  // compute the angle betweem both vectors. This is the amount to
  // rotate by.
  double angle = acos(vtkMath::Dot(lastVector, currVector));
  double crossVector[3];
  vtkMath::Cross(lastVector, currVector, crossVector);

  double aboutAxis[3];
  const int rcPlaneIdx = this->ResliceCursorActor->GetCursorAlgorithm()->GetReslicePlaneNormal();
  vtkPlane* normalPlane = rc->GetPlane(rcPlaneIdx);
  normalPlane->GetNormal(aboutAxis);
  const double align = vtkMath::Dot(aboutAxis, crossVector);
  const double sign = align > 0 ? 1.0 : -1.0;
  angle *= sign;

  if (angle == 0)
  {
    return 0;
  }

  this->RotateAxis(axis, angle);

  return angle;
}

//------------------------------------------------------------------------------
// Get the plane normal to the viewing axis.
vtkResliceCursorPolyDataAlgorithm* ResliceCursorLineRepresentation::GetCursorAlgorithm()
{
  return this->ResliceCursorActor->GetCursorAlgorithm();
}

//------------------------------------------------------------------------------
void ResliceCursorLineRepresentation::RotateAxis(int axis, double angle)
{
  vtkResliceCursor* rc = this->GetResliceCursor();
  vtkPlane* planeToBeRotated = rc->GetPlane(axis);
  double* viewUp = rc->GetViewUp(axis);

  const int rcPlaneIdx = this->ResliceCursorActor->GetCursorAlgorithm()->GetReslicePlaneNormal();

  vtkPlane* normalPlane = rc->GetPlane(rcPlaneIdx);

  double vectorToBeRotated[3], aboutAxis[3], rotatedVector[3];
  planeToBeRotated->GetNormal(vectorToBeRotated);
  normalPlane->GetNormal(aboutAxis);

  this->RotateVectorAboutVector(vectorToBeRotated, aboutAxis, angle, rotatedVector);
  this->RotateVectorAboutVector(viewUp, aboutAxis, angle, viewUp);
  planeToBeRotated->SetNormal(rotatedVector);
}

//------------------------------------------------------------------------------
void ResliceCursorLineRepresentation::RotateVectorAboutVector(double vectorToBeRotated[3],
  double axis[3], // vector about which we rotate
  double angle,   // angle in radians
  double o[3])
{
  vtkNew<vtkTransform> transform;
  transform->RotateWXYZ(vtkMath::DegreesFromRadians(angle), axis);
  transform->TransformVector(vectorToBeRotated, o);
}

//------------------------------------------------------------------------------
int ResliceCursorLineRepresentation::DisplayToReslicePlaneIntersection(
  double displayPos[2], double intersectionPos[3])
{
  // First compute the equivalent of this display point on the focal plane
  double fp[4], tmp1[4], camPos[4], eventFPpos[4];
  this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
  this->Renderer->GetActiveCamera()->GetPosition(camPos);
  fp[3] = 1.0;
  this->Renderer->SetWorldPoint(fp);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(tmp1);

  tmp1[0] = displayPos[0];
  tmp1[1] = displayPos[1];
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(eventFPpos);

  const int rcPlaneIdx = this->ResliceCursorActor->GetCursorAlgorithm()->GetReslicePlaneNormal();
  vtkPlane* normalPlane = this->GetResliceCursor()->GetPlane(rcPlaneIdx);

  double t;

  return normalPlane->IntersectWithLine(eventFPpos, camPos, t, intersectionPos);
}

//------------------------------------------------------------------------------
void ResliceCursorLineRepresentation::BuildRepresentation()
{
  if (this->GetMTime() > this->BuildTime ||
    this->GetResliceCursor()->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetVTKWindow() &&
      this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime))
  {

    this->BuildTime.Modified();
  }

  this->Superclass::BuildRepresentation();
}

//------------------------------------------------------------------------------
void ResliceCursorLineRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  this->ResliceCursorActor->ReleaseGraphicsResources(w);
  this->TexturePlaneActor->ReleaseGraphicsResources(w);
  this->ImageActor->ReleaseGraphicsResources(w);
  this->TextActor->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
int ResliceCursorLineRepresentation::RenderOverlay(vtkViewport* viewport)
{
  int count = 0;

  if (this->TexturePlaneActor->GetVisibility() && !this->UseImageActor)
  {
    count += this->TexturePlaneActor->RenderOverlay(viewport);
  }
  if (this->ImageActor->GetVisibility() && this->UseImageActor)
  {
    count += this->ImageActor->RenderOverlay(viewport);
  }
  if (this->DisplayText && this->TextActor->GetVisibility())
  {
    count += this->TextActor->RenderOverlay(viewport);
  }

  return count;
}

//------------------------------------------------------------------------------
void ResliceCursorLineRepresentation::SetUserMatrix(vtkMatrix4x4* m)
{
  this->TexturePlaneActor->SetUserMatrix(m);
  this->ResliceCursorActor->SetUserMatrix(m);
}

//------------------------------------------------------------------------------
int ResliceCursorLineRepresentation::RenderOpaqueGeometry(vtkViewport* viewport)
{
  this->BuildRepresentation();

  const int normalAxis = this->ResliceCursorActor->GetCursorAlgorithm()->GetReslicePlaneNormal();

  // When the reslice plane is changed, update the camera to look at the
  // normal to the reslice plane always.

  double fp[3], cp[3], n[3];
  this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
  this->Renderer->GetActiveCamera()->GetPosition(cp);
  this->GetResliceCursor()->GetPlane(normalAxis)->GetNormal(n);

  const double d = sqrt(vtkMath::Distance2BetweenPoints(cp, fp));
  double newCamPos[3] = { fp[0] + (d * n[0]), fp[1] + (d * n[1]), fp[2] + (d * n[2]) };
  this->Renderer->GetActiveCamera()->SetPosition(newCamPos);

  // intersect with the plane to get updated focal point
  double intersectionPos[3], t;
  this->GetResliceCursor()
    ->GetPlane(normalAxis)
    ->IntersectWithLine(fp, newCamPos, t, intersectionPos);
  this->Renderer->GetActiveCamera()->SetFocalPoint(intersectionPos);

  // Don't clip away any part of the data.
  this->Renderer->ResetCameraClippingRange();

  // Now Render all the actors.

  int count = 0;
  if (this->TexturePlaneActor->GetVisibility() && !this->UseImageActor)
  {
    count += this->TexturePlaneActor->RenderOpaqueGeometry(viewport);
  }
  if (this->ImageActor->GetVisibility() && this->UseImageActor)
  {
    count += this->ImageActor->RenderOpaqueGeometry(viewport);
  }
  count += this->ResliceCursorActor->RenderOpaqueGeometry(viewport);
  if (this->DisplayText && this->TextActor->GetVisibility())
  {
    count += this->TextActor->RenderOpaqueGeometry(viewport);
  }

  return count;
}

//------------------------------------------------------------------------------
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
double* ResliceCursorLineRepresentation::GetBounds()
{
  vtkMath::UninitializeBounds(this->InitialBounds);

  if (vtkResliceCursor* r = this->GetResliceCursor())
  {
    r->GetImage()->GetBounds(this->InitialBounds);
  }

  // vtkBoundingBox *bb = new vtkBoundingBox();
  // bb->AddBounds(this->ResliceCursorActor->GetBounds());
  // bb->AddBounds(this->TexturePlaneActor->GetBounds());
  // bb->GetBounds(bounds);
  // delete bb;

  return this->InitialBounds;
}

//------------------------------------------------------------------------------
int ResliceCursorLineRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  int count = 0;
  if (this->TexturePlaneActor->GetVisibility() && !this->UseImageActor)
  {
    count += this->TexturePlaneActor->RenderTranslucentPolygonalGeometry(viewport);
  }

  if (this->ImageActor->GetVisibility() && this->UseImageActor)
  {
    count += this->ImageActor->RenderTranslucentPolygonalGeometry(viewport);
  }

  count += this->ResliceCursorActor->RenderTranslucentPolygonalGeometry(viewport);

  return count;
}

//------------------------------------------------------------------------------
vtkTypeBool ResliceCursorLineRepresentation::HasTranslucentPolygonalGeometry()
{
  return (this->ResliceCursorActor->HasTranslucentPolygonalGeometry() ||
           (this->ImageActor->HasTranslucentPolygonalGeometry() && this->UseImageActor) ||
           (this->TexturePlaneActor->HasTranslucentPolygonalGeometry() && !this->UseImageActor))
    ? 1
    : 0;
}

//------------------------------------------------------------------------------
void ResliceCursorLineRepresentation::Highlight(int) {}

//------------------------------------------------------------------------------
void ResliceCursorLineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ResliceCursorActor: " << this->ResliceCursorActor << "\n";
  if (this->ResliceCursorActor)
  {
    this->ResliceCursorActor->PrintSelf(os, indent);
  }

  os << indent << "Picker: " << this->Picker << "\n";
  if (this->Picker)
  {
    this->Picker->PrintSelf(os, indent);
  }

  os << indent << "MatrixReslicedView: " << this->MatrixReslicedView << "\n";
  if (this->MatrixReslicedView)
  {
    this->MatrixReslicedView->PrintSelf(os, indent);
  }

  os << indent << "MatrixView: " << this->MatrixView << "\n";
  if (this->MatrixView)
  {
    this->MatrixView->PrintSelf(os, indent);
  }

  os << indent << "MatrixReslice: " << this->MatrixReslice << "\n";
  if (this->MatrixReslice)
  {
    this->MatrixReslice->PrintSelf(os, indent);
  }

  // this->StartPickPosition;
  // this->StartCenterPosition;
}
