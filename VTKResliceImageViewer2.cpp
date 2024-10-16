﻿/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResliceImageViewer2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBoundedPlanePointPlacer.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageMapToWindowLevelColors.h"
#include "vtkImageReslice.h"
#include "vtkInteractorStyleImage.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkResliceCursor.h"
#include "vtkResliceCursorActor.h"
#include "ResliceCursorLineRepresentation.h"
#include "vtkResliceCursorPolyDataAlgorithm.h"
#include "vtkResliceCursorThickLineRepresentation.h"
#include "vtkResliceCursorWidget.h"
#include "vtkResliceImageViewer2.h"
#include "vtkResliceImageViewerMeasurements.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkResliceImageViewer2);

//------------------------------------------------------------------------------
// This class is used to scroll slices with the scroll bar. In the case of MPR
// view, it moves one "normalized spacing" in the direction of the normal to
// the resliced plane, provided the new center will continue to lie within the
// volume.
class vtkResliceImageViewerScrollCallback : public vtkCommand {
public:
    static vtkResliceImageViewerScrollCallback* New() { return new vtkResliceImageViewerScrollCallback; }

    void Execute(vtkObject*, unsigned long ev, void*) override
    {
        if (!this->Viewer->GetSliceScrollOnMouseWheel()) {
            return;
        }

        // Do not process if any modifiers are ON
        if (this->Viewer->GetInteractor()->GetShiftKey() || this->Viewer->GetInteractor()->GetControlKey() ||
            this->Viewer->GetInteractor()->GetAltKey()) {
            return;
        }

        // forwards or backwards
        int sign = (ev == vtkCommand::MouseWheelForwardEvent) ? 1 : -1;
        this->Viewer->IncrementSlice(sign);

        // Abort further event processing for the scroll.
        this->SetAbortFlag(1);
    }

    vtkResliceImageViewerScrollCallback() : Viewer(nullptr) {}
    vtkResliceImageViewer2* Viewer;
};

//------------------------------------------------------------------------------
vtkResliceImageViewer2::vtkResliceImageViewer2()
{
    // Default is to not use the reslice cursor widget, ie use fast
    // 3D texture mapping to display slices.
    this->ResliceMode = vtkResliceImageViewer2::RESLICE_AXIS_ALIGNED;

    // Set up the reslice cursor widget, should it be used.

    this->ResliceCursorWidget = vtkResliceCursorWidget::New();

    vtkSmartPointer<vtkResliceCursor> resliceCursor = vtkSmartPointer<vtkResliceCursor>::New();
    resliceCursor->SetThickMode(0);
    resliceCursor->SetThickness(10, 10, 10);

    vtkSmartPointer<ResliceCursorLineRepresentation> resliceCursorRep =
        vtkSmartPointer<ResliceCursorLineRepresentation>::New();
    resliceCursorRep->GetResliceCursorActor()->GetCursorAlgorithm()->SetResliceCursor(resliceCursor);
    resliceCursorRep->GetResliceCursorActor()->GetCursorAlgorithm()->SetReslicePlaneNormal(this->SliceOrientation);
    this->ResliceCursorWidget->SetRepresentation(resliceCursorRep);

    this->PointPlacer = vtkBoundedPlanePointPlacer::New();

    // this->Measurements = vtkResliceImageViewerMeasurements::New();
    // this->Measurements->SetResliceImageViewer(this);

    this->ScrollCallback = vtkResliceImageViewerScrollCallback::New();
    this->ScrollCallback->Viewer = this;
    this->SliceScrollOnMouseWheel = 1;

    this->InstallPipeline();
}

//------------------------------------------------------------------------------
vtkResliceImageViewer2::~vtkResliceImageViewer2()
{
    // this->Measurements->Delete();

    if (this->ResliceCursorWidget) {
        this->ResliceCursorWidget->Delete();
        this->ResliceCursorWidget = nullptr;
    }

    this->PointPlacer->Delete();
    this->ScrollCallback->Delete();
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::SetThickMode(int t)
{
    vtkSmartPointer<vtkResliceCursor> rc = this->GetResliceCursor();

    if (t == this->GetThickMode()) {
        return;
    }

    vtkSmartPointer<ResliceCursorLineRepresentation> resliceCursorRepOld =
        ResliceCursorLineRepresentation::SafeDownCast(this->ResliceCursorWidget->GetRepresentation());
    vtkSmartPointer<vtkResliceCursorLineRepresentation> resliceCursorRepNew;

    this->GetResliceCursor()->SetThickMode(t);

    if (t) {
        resliceCursorRepNew = vtkSmartPointer<vtkResliceCursorThickLineRepresentation>::New();
    }
    else {
        resliceCursorRepNew = vtkSmartPointer<ResliceCursorLineRepresentation>::New();
    }

    int e = this->ResliceCursorWidget->GetEnabled();
    this->ResliceCursorWidget->SetEnabled(0);

    resliceCursorRepNew->GetResliceCursorActor()->GetCursorAlgorithm()->SetResliceCursor(rc);
    resliceCursorRepNew->GetResliceCursorActor()->GetCursorAlgorithm()->SetReslicePlaneNormal(this->SliceOrientation);
    this->ResliceCursorWidget->SetRepresentation(resliceCursorRepNew);
    resliceCursorRepNew->SetLookupTable(resliceCursorRepOld->GetLookupTable());

    resliceCursorRepNew->SetWindowLevel(resliceCursorRepOld->GetWindow(), resliceCursorRepOld->GetLevel(), 1);

    this->ResliceCursorWidget->SetEnabled(e);
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::SetResliceCursor(vtkResliceCursor* rc)
{
    vtkResliceCursorRepresentation* rep =
        vtkResliceCursorRepresentation::SafeDownCast(this->GetResliceCursorWidget()->GetRepresentation());
    rep->GetCursorAlgorithm()->SetResliceCursor(rc);

    // Rehook the observer to this reslice cursor.
    // this->Measurements->SetResliceImageViewer(this);
}

//------------------------------------------------------------------------------
int vtkResliceImageViewer2::GetThickMode()
{
    return (vtkResliceCursorThickLineRepresentation::SafeDownCast(this->ResliceCursorWidget->GetRepresentation())) ? 1
                                                                                                                   : 0;
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::SetLookupTable(vtkScalarsToColors* l)
{
    if (vtkResliceCursorRepresentation* rep =
            vtkResliceCursorRepresentation::SafeDownCast(this->ResliceCursorWidget->GetRepresentation())) {
        rep->SetLookupTable(l);
    }

    if (this->WindowLevel) {
        this->WindowLevel->SetLookupTable(l);
        this->WindowLevel->SetOutputFormatToRGBA();
        this->WindowLevel->PassAlphaToOutputOn();
    }
}

//------------------------------------------------------------------------------
vtkScalarsToColors* vtkResliceImageViewer2::GetLookupTable()
{
    if (vtkResliceCursorRepresentation* rep =
            vtkResliceCursorRepresentation::SafeDownCast(this->ResliceCursorWidget->GetRepresentation())) {
        return rep->GetLookupTable();
    }

    return nullptr;
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::UpdateOrientation()
{
    // Set the camera position

    vtkCamera* cam = this->Renderer ? this->Renderer->GetActiveCamera() : nullptr;
    if (cam) {
        switch (this->SliceOrientation) {
            case vtkImageViewer2::SLICE_ORIENTATION_XY:
                cam->SetFocalPoint(0, 0, 0);
                cam->SetPosition(0, 0, -1);  // -1 if medical ?
                cam->SetViewUp(0, -1, 0);
                break;

            case vtkImageViewer2::SLICE_ORIENTATION_XZ:
                cam->SetFocalPoint(0, 0, 0);
                cam->SetPosition(0, -1, 0);  // 1 if medical ?
                cam->SetViewUp(0, 0, 1);
                break;

            case vtkImageViewer2::SLICE_ORIENTATION_YZ:
                cam->SetFocalPoint(0, 0, 0);
                cam->SetPosition(1, 0, 0);  // -1 if medical ?
                cam->SetViewUp(0, 0, 1);
                break;
        }
    }
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::UpdateDisplayExtent()
{
    // Only update the display extent in axis aligned mode

    if (this->ResliceMode == RESLICE_AXIS_ALIGNED) {
        this->Superclass::UpdateDisplayExtent();
    }
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::InstallPipeline()
{
    this->Superclass::InstallPipeline();

    if (this->Interactor) {

        this->ResliceCursorWidget->SetInteractor(this->Interactor);

        // Observe the scroll for slice manipulation at a higher priority
        // than the interactor style.
        this->Interactor->RemoveObserver(this->ScrollCallback);
        this->Interactor->AddObserver(vtkCommand::MouseWheelForwardEvent, this->ScrollCallback, 0.55);
        this->Interactor->AddObserver(vtkCommand::MouseWheelBackwardEvent, this->ScrollCallback, 0.55);
    }

    if (this->Renderer) {
        this->ResliceCursorWidget->SetDefaultRenderer(this->Renderer);
        vtkCamera* cam = this->Renderer->GetActiveCamera();
        cam->ParallelProjectionOn();
    }

    if (this->ResliceMode == RESLICE_OBLIQUE) {
        this->ResliceCursorWidget->SetEnabled(1);
        this->ImageActor->SetVisibility(0);
        this->UpdateOrientation();

        double bounds[6] = {0, 1, 0, 1, 0, 1};

        vtkCamera* cam = this->Renderer->GetActiveCamera();
        double onespacing[3] = {1, 1, 1};
        double* spacing = onespacing;
        if (this->GetResliceCursor()->GetImage()) {
            this->GetResliceCursor()->GetImage()->GetBounds(bounds);
            spacing = this->GetResliceCursor()->GetImage()->GetSpacing();
        }
        double avg_spacing = (spacing[0] + spacing[1] + spacing[2]) / 3.0;
        cam->SetClippingRange(bounds[this->SliceOrientation * 2] - 100 * avg_spacing,
                              bounds[this->SliceOrientation * 2 + 1] + 100 * avg_spacing);
    }
    else {
        this->ResliceCursorWidget->SetEnabled(0);
        this->ImageActor->SetVisibility(1);
        this->UpdateOrientation();
    }

    if (this->WindowLevel) {
        this->WindowLevel->SetLookupTable(this->GetLookupTable());
    }
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::UnInstallPipeline()
{
    this->ResliceCursorWidget->SetEnabled(0);

    if (this->Interactor) {
        this->Interactor->RemoveObserver(this->ScrollCallback);
    }

    this->Superclass::UnInstallPipeline();

}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::UpdatePointPlacer()
{
    if (this->ResliceMode == RESLICE_OBLIQUE) {
        this->PointPlacer->SetProjectionNormalToOblique();
        if (vtkResliceCursorRepresentation* rep =
                vtkResliceCursorRepresentation::SafeDownCast(this->ResliceCursorWidget->GetRepresentation())) {
            const int planeOrientation = rep->GetCursorAlgorithm()->GetReslicePlaneNormal();
            vtkPlane* plane = this->GetResliceCursor()->GetPlane(planeOrientation);
            this->PointPlacer->SetObliquePlane(plane);
        }
    }
    else {
        if (!this->WindowLevel->GetInput()) {
            return;
        }

        vtkImageData* input = this->ImageActor->GetInput();
        if (!input) {
            return;
        }

        double spacing[3];
        input->GetSpacing(spacing);

        double origin[3];
        input->GetOrigin(origin);

        double bounds[6];
        this->ImageActor->GetBounds(bounds);

        int displayExtent[6];
        this->ImageActor->GetDisplayExtent(displayExtent);

        int axis = vtkBoundedPlanePointPlacer::XAxis;
        double position = 0.0;
        if (displayExtent[0] == displayExtent[1]) {
            axis = vtkBoundedPlanePointPlacer::XAxis;
            position = origin[0] + displayExtent[0] * spacing[0];
        }
        else if (displayExtent[2] == displayExtent[3]) {
            axis = vtkBoundedPlanePointPlacer::YAxis;
            position = origin[1] + displayExtent[2] * spacing[1];
        }
        else if (displayExtent[4] == displayExtent[5]) {
            axis = vtkBoundedPlanePointPlacer::ZAxis;
            position = origin[2] + displayExtent[4] * spacing[2];
        }

        this->PointPlacer->SetProjectionNormal(axis);
        this->PointPlacer->SetProjectionPosition(position);
    }
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::Render()
{
    if (!this->WindowLevel->GetInput()) {
        return;
    }

    this->UpdatePointPlacer();

    this->Superclass::Render();
}

//------------------------------------------------------------------------------
vtkResliceCursor* vtkResliceImageViewer2::GetResliceCursor()
{
    if (vtkResliceCursorRepresentation* rep =
            vtkResliceCursorRepresentation::SafeDownCast(this->ResliceCursorWidget->GetRepresentation())) {
        return rep->GetResliceCursor();
    }

    return nullptr;
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::SetInputData(vtkImageData* in)
{
    if (!in) {
        return;
    }

    this->WindowLevel->SetInputData(in);
    this->GetResliceCursor()->SetImage(in);
    this->GetResliceCursor()->SetCenter(in->GetCenter());
    this->UpdateDisplayExtent();

    double range[2];
    in->GetScalarRange(range);
    if (vtkResliceCursorRepresentation* rep =
            vtkResliceCursorRepresentation::SafeDownCast(this->ResliceCursorWidget->GetRepresentation())) {
        if (vtkImageReslice* reslice = vtkImageReslice::SafeDownCast(rep->GetReslice())) {
            // default background color is the min value of the image scalar range
            reslice->SetBackgroundColor(range[0], range[0], range[0], range[0]);
            this->SetColorWindow(range[1] - range[0]);
            this->SetColorLevel((range[0] + range[1]) / 2.0);
        }
    }
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::SetInputConnection(vtkAlgorithmOutput* input)
{
    vtkErrorMacro(<< "Use SetInputData instead. ");
    this->WindowLevel->SetInputConnection(input);
    this->UpdateDisplayExtent();
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::SetResliceMode(int r)
{
    if (r == this->ResliceMode) {
        return;
    }

    this->ResliceMode = r;
    this->Modified();

    this->InstallPipeline();
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::SetColorWindow(double w)
{
    double rmin = this->GetColorLevel() - 0.5 * fabs(w);
    double rmax = rmin + fabs(w);
    this->GetLookupTable()->SetRange(rmin, rmax);

    this->WindowLevel->SetWindow(w);
    if (vtkResliceCursorRepresentation* rep =
            vtkResliceCursorRepresentation::SafeDownCast(this->ResliceCursorWidget->GetRepresentation())) {
        rep->SetWindowLevel(w, rep->GetLevel(), 1);
    }
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::SetColorLevel(double w)
{
    double rmin = w - 0.5 * fabs(this->GetColorWindow());
    double rmax = rmin + fabs(this->GetColorWindow());
    this->GetLookupTable()->SetRange(rmin, rmax);

    this->WindowLevel->SetLevel(w);
    if (vtkResliceCursorRepresentation* rep =
            vtkResliceCursorRepresentation::SafeDownCast(this->ResliceCursorWidget->GetRepresentation())) {
        rep->SetWindowLevel(rep->GetWindow(), w, 1);
    }
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::Reset()
{
    this->ResliceCursorWidget->ResetResliceCursor();
}

//------------------------------------------------------------------------------
vtkPlane* vtkResliceImageViewer2::GetReslicePlane()
{
    // Get the reslice plane
    if (vtkResliceCursorRepresentation* rep =
            vtkResliceCursorRepresentation::SafeDownCast(this->ResliceCursorWidget->GetRepresentation())) {
        const int planeOrientation = rep->GetCursorAlgorithm()->GetReslicePlaneNormal();
        vtkPlane* plane = this->GetResliceCursor()->GetPlane(planeOrientation);
        return plane;
    }

    return nullptr;
}

//------------------------------------------------------------------------------
double vtkResliceImageViewer2::GetInterSliceSpacingInResliceMode()
{
    double n[3], imageSpacing[3], resliceSpacing = 0;

    if (vtkPlane* plane = this->GetReslicePlane()) {
        plane->GetNormal(n);
        this->GetResliceCursor()->GetImage()->GetSpacing(imageSpacing);
        resliceSpacing = fabs(vtkMath::Dot(n, imageSpacing));
    }

    return resliceSpacing;
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::IncrementSlice(int inc)
{
    if (this->GetResliceMode() == vtkResliceImageViewer2::RESLICE_AXIS_ALIGNED) {
        int oldSlice = this->GetSlice();
        this->SetSlice(this->GetSlice() + inc);
        if (this->GetSlice() != oldSlice) {
            this->InvokeEvent(vtkResliceImageViewer2::SliceChangedEvent, nullptr);
            this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
        }
    }
    else {
        int oldSlice = this->GetSlice();
        this->SetSlice(this->GetSlice() + inc);
        // if (this->GetSlice() != oldSlice) {
        //     this->InvokeEvent(vtkResliceImageViewer2::SliceChangedEvent, nullptr);
        //     this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
        // }

        if (vtkPlane* p = this->GetReslicePlane()) {
            double n[3], c[3], bounds[6];
            p->GetNormal(n);
            const double spacing = this->GetInterSliceSpacingInResliceMode() * inc;
            this->GetResliceCursor()->GetCenter(c);
            vtkMath::MultiplyScalar(n, spacing);
            c[0] += n[0];
            c[1] += n[1];
            c[2] += n[2];

            // If the new center is inside, put it there...
            if (vtkImageData* image = this->GetResliceCursor()->GetImage()) {
                image->GetBounds(bounds);
                if (c[0] >= bounds[0] && c[0] <= bounds[1] && c[1] >= bounds[2] && c[1] <= bounds[3] &&
                    c[2] >= bounds[4] && c[2] <= bounds[5]) {
                    this->GetResliceCursor()->SetCenter(c);

                    this->InvokeEvent(vtkResliceImageViewer2::SliceChangedEvent, nullptr);
                    this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
void vtkResliceImageViewer2::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);

    os << indent << "ResliceCursorWidget:\n";
    this->ResliceCursorWidget->PrintSelf(os, indent.GetNextIndent());
    os << indent << "ResliceMode: " << this->ResliceMode << endl;
    os << indent << "SliceScrollOnMouseWheel: " << this->SliceScrollOnMouseWheel << endl;
    os << indent << "Point Placer: ";
    this->PointPlacer->PrintSelf(os, indent.GetNextIndent());
    os << indent << "Measurements: ";
    this->Measurements->PrintSelf(os, indent.GetNextIndent());
    os << indent << "Interactor: " << this->Interactor << "\n";
    if (this->Interactor) {
        this->Interactor->PrintSelf(os, indent.GetNextIndent());
    }
}
