#include "VTKResliceCursorCallback.h"

#include <vtkCamera.h>
#include <vtkCoordinate.h>
#include <vtkMatrix4x4.h>
#include <vtkPlaneSource.h>
#include <vtkResliceCursor.h>
#include <vtkResliceCursorActor.h>
#include <vtkResliceCursorLineRepresentation.h>
#include <vtkResliceCursorPolyDataAlgorithm.h>
#include <vtkResliceCursorWidget.h>
#include <vtkTransform.h>
#include <vtkImageData.h>
#include <QDebug>
//--------------------------------------------------------------------------------------
// VTKView2DCallBack class implementation
// xxxx
//--------------------------------------------------------------------------------------

VTKResliceCursorCallback::VTKResliceCursorCallback()
{
}

VTKResliceCursorCallback::~VTKResliceCursorCallback()
{
}

void VTKResliceCursorCallback::setMPRXWidget(VTKRender2DWidget *pwidget)
{
    m_render2DWidget[0] = pwidget;

    pwidget->getResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceAxesChangedEvent, this);
    pwidget->getResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::WindowLevelEvent, this);
    pwidget->getResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceThicknessChangedEvent, this);
    pwidget->getResliceCursorWidget()->AddObserver(vtkCommand::EndInteractionEvent, this);
    pwidget->getResliceCursorWidget()->AddObserver(vtkCommand::StartInteractionEvent, this);

    pwidget->getResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResetCursorEvent, this);
    // m_impl->m_riw->GetInteractorStyle()->AddObserver(vtkCommand::WindowLevelEvent, pcallback);
    pwidget->getResliceImageViewer2()->AddObserver(vtkResliceImageViewer2::SliceChangedEvent,
                                                   this);  // 添加这句话，就能实现功能1的效果；
}
void VTKResliceCursorCallback::setMPRYWidget(VTKRender2DWidget *pwidget)
{
    m_render2DWidget[1] = pwidget;

    pwidget->getResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceAxesChangedEvent, this);
    pwidget->getResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::WindowLevelEvent, this);
    pwidget->getResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceThicknessChangedEvent, this);
    pwidget->getResliceCursorWidget()->AddObserver(vtkCommand::EndInteractionEvent, this);
    pwidget->getResliceCursorWidget()->AddObserver(vtkCommand::StartInteractionEvent, this);

    pwidget->getResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResetCursorEvent, this);
    // m_impl->m_riw->GetInteractorStyle()->AddObserver(vtkCommand::WindowLevelEvent, pcallback);
    pwidget->getResliceImageViewer2()->AddObserver(vtkResliceImageViewer2::SliceChangedEvent,
                                                   this);  // 添加这句话，就能实现功能1的效果；
}
void VTKResliceCursorCallback::setMPRZWidget(VTKRender2DWidget *pwidget)
{
    m_render2DWidget[2] = pwidget;

    pwidget->getResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceAxesChangedEvent, this);
    pwidget->getResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::WindowLevelEvent, this);
    pwidget->getResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceThicknessChangedEvent, this);
    pwidget->getResliceCursorWidget()->AddObserver(vtkCommand::EndInteractionEvent, this);
    pwidget->getResliceCursorWidget()->AddObserver(vtkCommand::StartInteractionEvent, this);

    pwidget->getResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResetCursorEvent, this);
    // m_impl->m_riw->GetInteractorStyle()->AddObserver(vtkCommand::WindowLevelEvent, pcallback);
    pwidget->getResliceImageViewer2()->AddObserver(vtkResliceImageViewer2::SliceChangedEvent,
                                                   this);  // 添加这句话，就能实现功能1的效果；
}

void VTKResliceCursorCallback::Execute(vtkObject *caller, unsigned long eventId, void *callData)
{
    int index = 0;
    vtkResliceCursorWidget* rcw = dynamic_cast<vtkResliceCursorWidget*>(caller);
    if(!rcw)
        return;
    if (m_render2DWidget[1]->getResliceCursorWidget() == rcw) {
        index = 1;
    }
    else if (m_render2DWidget[2]->getResliceCursorWidget() == rcw) {
        index = 2;
    }

    if (eventId == vtkResliceCursorWidget::WindowLevelEvent || eventId == vtkCommand::WindowLevelEvent ||
        eventId == vtkResliceCursorWidget::ResliceThicknessChangedEvent) {
        for (int i = 0; i < 3; i++) {
            this->m_render2DWidget[i]->getResliceCursorWidget()->Render();
        }
        return;
    }

    if (eventId == vtkCommand::StartInteractionEvent) {
        this->m_render2DWidget[0]->m_bIsMouseMove = false;
        this->m_render2DWidget[1]->m_bIsMouseMove = false;
        this->m_render2DWidget[2]->m_bIsMouseMove = false;
    }
    if (eventId == vtkCommand::EndInteractionEvent && rcw) {
        
        if (!this->m_render2DWidget[index]->m_bIsMouseMove) {

            vtkRenderWindowInteractor *pInteractor = rcw->GetInteractor();
            int x = pInteractor->GetEventPosition()[0];
            int y = pInteractor->GetEventPosition()[1];
            int z = pInteractor->GetEventPosition()[2];

            vtkRenderer *renderer = m_render2DWidget[index]->getRenderer();

            double *center = m_render2DWidget[index]->getResliceCursor()->GetCenter();
            renderer->SetDisplayPoint(x, y, 0);
            renderer->DisplayToWorld();
            double world_point[3];
            world_point[0] = (renderer->GetWorldPoint())[0];
            world_point[1] = (renderer->GetWorldPoint())[1];
            world_point[2] = (renderer->GetWorldPoint())[2];
            switch (index) {
                case 0:
                    world_point[1] = center[1];
                    break;
                case 1:
                    world_point[2] = center[2];
                    break;
                case 2:
                    world_point[0] = center[0];
                    break;
            }
            m_render2DWidget[index]->getResliceCursor()->SetCenter(world_point);
        }
    }


    if (eventId == vtkResliceCursorWidget::ResliceAxesChangedEvent)
    {
        vtkResliceCursorLineRepresentation *rep =
            dynamic_cast<vtkResliceCursorLineRepresentation *>(rcw->GetRepresentation());
        vtkResliceCursor *cursor = rep->GetResliceCursorActor()->GetCursorAlgorithm()->GetResliceCursor();
        double *center = cursor->GetCenter();
        double imageindex[3] = {0, 0, 0};
        cursor->GetImage()->TransformPhysicalPointToContinuousIndex(center, imageindex);
        if(index==0)
        {
            m_render2DWidget[1]->setImageSlice(imageindex[1]);
            m_render2DWidget[2]->setImageSlice(imageindex[0]);
        }
        else if(index==1)
        {
            m_render2DWidget[0]->setImageSlice(imageindex[2]);
            m_render2DWidget[2]->setImageSlice(imageindex[0]);
        }
        else
        {
            m_render2DWidget[0]->setImageSlice(imageindex[2]);
            m_render2DWidget[1]->setImageSlice(imageindex[1]);
        }
    }

    for (int i = 0; i < 3; i++) {
        this->m_render2DWidget[i]->getResliceCursorWidget()->Render();
    }
}
