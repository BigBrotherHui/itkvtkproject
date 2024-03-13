
#include "VTKRender3DWidget.h"

#include <vtkAnnotatedCubeActor.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkInteractorStyleMultiTouchCamera.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkNew.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPolyDataMapper.h>
#include <vtkPropCollection.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

#include "ui_VTKRender3DWidget.h"

class VTKRender3DWidget::VTKRender3DWidgetImpl {
public:
    VTKRender3DWidgetImpl() {}
    ~VTKRender3DWidgetImpl() {}

    // 获取3D渲染场景包围盒
    void getPropsBound(double bound[6])
    {
        bound[0] = VTK_FLOAT_MAX;
        bound[2] = VTK_FLOAT_MAX;
        bound[4] = VTK_FLOAT_MAX;
        bound[1] = VTK_FLOAT_MIN;
        bound[3] = VTK_FLOAT_MIN;
        bound[5] = VTK_FLOAT_MIN;

        vtkPropCollection *actors = m_3DRenderer->GetViewProps();
        if (actors->GetNumberOfItems() == 0) {
            return;
        }

        double p[6] = {0};
        for (actors->InitTraversal(); vtkProp *prop = actors->GetNextProp();) {
            if (vtkActor *actor = vtkActor::SafeDownCast(prop)) {
                actor->GetBounds();
                bound[0] = std::min(bound[0], p[0]);
                bound[2] = std::min(bound[2], p[2]);
                bound[4] = std::min(bound[4], p[4]);

                bound[1] = std::max(bound[1], p[1]);
                bound[3] = std::max(bound[3], p[3]);
                bound[5] = std::max(bound[5], p[5]);
            }
        }
    }

public:
    // vtkNew<vtkRenderer> m_backgroundRender;                    // 背景渲染器
    vtkNew<vtkRenderer> m_3DRenderer;  // 3D渲染器
    // vtkNew<vtkInteractorStyleTrackballCamera> m_trackballcamera;  // 三维相机交互样式
    // vtkNew<vtkAxesActor> m_axes;                                  // 坐标方向箭头
    vtkNew<vtkAnnotatedCubeActor> m_axes;             // 坐标方向箭头
    vtkNew<vtkOrientationMarkerWidget> m_markwidget;  // 坐标方向指示器
};

VTKRender3DWidget::VTKRender3DWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::VTKRender3DWidget), m_impl(std::make_unique<VTKRender3DWidgetImpl>())
{
    ui->setupUi(this);

    this->initialRender();
    this->initPatientInfo();
    this->setPatientInfoVisible(false);
    ui->m_patientInfoPanel->setGeometry(10, 10, 200, 200);
    m_volume = vtkSmartPointer<vtkVolume>::New();
    m_colorFunc = vtkSmartPointer<vtkColorTransferFunction>::New();
    m_opacityFunc = vtkSmartPointer<vtkPiecewiseFunction>::New();
    m_volMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
    m_volProps = vtkSmartPointer<vtkVolumeProperty>::New();
    m_gradientOpacityFunc = vtkSmartPointer<vtkPiecewiseFunction>::New();
    m_gradientOpacityFunc->AddPoint(0., 1.);
    m_gradientOpacityFunc->AddPoint(255., 1.);
    m_volProps->SetInterpolationTypeToLinear();
    m_volProps->SetScalarOpacity(m_opacityFunc);
    m_volProps->SetGradientOpacity(m_gradientOpacityFunc);
    m_volProps->SetColor(m_colorFunc);
    m_volume->SetMapper(m_volMapper);
    m_volume->SetProperty(m_volProps);
    m_volMapper->SetBlendModeToComposite();
    m_volMapper->SetAutoAdjustSampleDistances(0);
    m_volMapper->SetSampleDistance(4);
    m_volProps->ShadeOn();
    m_volProps->SetAmbient(0.15);
    m_volProps->SetDiffuse(0.9);
    m_volProps->SetSpecular(0.15);
    m_volProps->SetSpecularPower(10.0);
    m_volProps->SetInterpolationTypeToLinear();

    addActor("volume", m_volume);
}

VTKRender3DWidget::~VTKRender3DWidget()
{
    delete ui;
}

void VTKRender3DWidget::setImageData(vtkImageData* pImageData)
{
    if (!pImageData)
        return;
    m_volMapper->SetInputData(pImageData);

    m_opacityFunc->RemoveAllPoints();
    m_opacityFunc->AddPoint(70, 0.00);
    m_opacityFunc->AddPoint(90, 0.40);
    m_opacityFunc->AddPoint(180, 0.60);
    //设置梯度不透明属性
    m_gradientOpacityFunc->RemoveAllPoints();
    m_gradientOpacityFunc->AddPoint(10, 0.0);
    m_gradientOpacityFunc->AddPoint(90, 0.5);
    m_gradientOpacityFunc->AddPoint(100, 1.0);
    //设置颜色属性
    m_colorFunc->RemoveAllPoints();
    m_colorFunc->AddRGBPoint(0.000, 0.00, 0.00, 0.00);
    m_colorFunc->AddRGBPoint(64.00, 1.00, 0.52, 0.30);
    m_colorFunc->AddRGBPoint(190.0, 1.00, 1.00, 1.00);
    m_colorFunc->AddRGBPoint(220.0, 0.20, 0.20, 0.20);
    m_impl->m_3DRenderer->ResetCamera();
    ui->qvtkwidget->renderWindow()->Render();
}

void VTKRender3DWidget::setBlendMode(int index)
{
    m_volMapper->SetBlendMode(index);
}

void VTKRender3DWidget::switchCameraView(ViewPort3D viewPort3D)
{
    this->zoomView();

    vtkCamera* camera = m_impl->m_3DRenderer->GetActiveCamera();

    // 获取相机的位置、焦点和视上向量
    double position[3];
    double focalPoint[3];
    double viewUp[3];
    camera->GetPosition(position);
    camera->GetFocalPoint(focalPoint);
    camera->GetViewUp(viewUp);

    // 根据方向参数，计算新的位置、焦点和视上向量
    switch (viewPort3D) {
    case ViewPort3D::VIEWPORT3D_BACK:
        position[0] = focalPoint[0];
        position[1] = focalPoint[1] + camera->GetDistance();
        position[2] = focalPoint[2];
        viewUp[0] = 0.0;
        viewUp[1] = 0.0;
        viewUp[2] = 1.0;
        break;
    case ViewPort3D::VIEWPORT3D_FRONT:
        position[0] = focalPoint[0];
        position[1] = focalPoint[1] - camera->GetDistance();
        position[2] = focalPoint[2];
        viewUp[0] = 0.0;
        viewUp[1] = 0.0;
        viewUp[2] = 1.0;
        break;
    case ViewPort3D::VIEWPORT3D_BOTTOM:
        position[0] = focalPoint[0];
        position[1] = focalPoint[1];
        position[2] = focalPoint[2] - camera->GetDistance();
        viewUp[0] = 0.0;
        viewUp[1] = 1.0;
        viewUp[2] = 0.0;
        break;
    case ViewPort3D::VIEWPORT3D_TOP:
        position[0] = focalPoint[0];
        position[1] = focalPoint[1];
        position[2] = focalPoint[2] + camera->GetDistance();
        viewUp[0] = 0.0;
        viewUp[1] = 1.0;
        viewUp[2] = 0.0;
        break;
    case ViewPort3D::VIEWPORT3D_LEFT:
        position[0] = focalPoint[0] + camera->GetDistance();
        position[1] = focalPoint[1];
        position[2] = focalPoint[2];
        viewUp[0] = 0.0;
        viewUp[1] = 0.0;
        viewUp[2] = 1.0;
        break;
    case ViewPort3D::VIEWPORT3D_RIGHT:
        position[0] = focalPoint[0] - camera->GetDistance();
        position[1] = focalPoint[1];
        position[2] = focalPoint[2];
        viewUp[0] = 0.0;
        viewUp[1] = 0.0;
        viewUp[2] = 1.0;
        break;
    }

    // 设置新的位置、焦点和视上向量
    camera->SetPosition(position);
    camera->SetFocalPoint(focalPoint);
    camera->SetViewUp(viewUp);

    m_impl->m_3DRenderer->ResetCameraClippingRange();
    ui->qvtkwidget->renderWindow()->Render();
}

void VTKRender3DWidget::initialRender()
{
    // // 3D渲染窗口背景颜色调整，当前为蓝色渐变，可以修改参数值来调整背景色
    // m_impl->m_backgroundRender->SetBackground(0.1, 0.3, 0.8);
    // m_impl->m_backgroundRender->GradientBackgroundOn();
    // m_impl->m_backgroundRender->SetBackground2(0.0, 0.2, 0.6);
    // m_impl->m_backgroundRender->InteractiveOff();
    // m_impl->m_backgroundRender->SetLayer(0);

    // m_impl->m_3DRenderer->BackingStoreOff();
    // m_impl->m_3DRenderer->UseDepthPeelingOn();
    // m_impl->m_3DRenderer->SetOcclusionRatio(0.1);
    // m_impl->m_3DRenderer->SetMaximumNumberOfPeels(0);
    // m_impl->m_3DRenderer->SetLayer(1);

    // 设置背景色
    m_impl->m_3DRenderer->SetBackground(0, 0, 0);
    // m_impl->m_3DRenderer->GradientBackgroundOn();
    // m_impl->m_3DRenderer->SetBackground2(0.0, 0.2, 0.6);

    ui->qvtkwidget->renderWindow()->AddRenderer(m_impl->m_3DRenderer);
    // ui->qvtkwidget->renderWindow()->AddRenderer(m_impl->m_backgroundRender);

    // 添加3D坐标轴小部件
    // m_impl->m_axes->SetXPlusFaceText("L");
    // m_impl->m_axes->SetXMinusFaceText("R");
    // m_impl->m_axes->SetYMinusFaceText("A");
    // m_impl->m_axes->SetYPlusFaceText("P");
    // m_impl->m_axes->SetZMinusFaceText("I");
    // m_impl->m_axes->SetZPlusFaceText("S");
    // m_impl->m_markwidget->SetOutlineColor(1, 1, 1);
    // m_impl->m_markwidget->SetOrientationMarker(m_impl->m_axes);
    // m_impl->m_markwidget->SetInteractor(interactor);
    // m_impl->m_markwidget->SetViewport(0.9, 0.05, 1.0, 0.15);
    // m_impl->m_markwidget->SetEnabled(1);
    // m_impl->m_markwidget->InteractiveOff();
    vtkCamera *pCamera = m_impl->m_3DRenderer->GetActiveCamera();
    ui->qvtkwidget->renderWindow()->Render();
}

void VTKRender3DWidget::zoomView(bool reset)
{
    m_impl->m_3DRenderer->ResetCamera();
    ui->qvtkwidget->renderWindow()->Render();
}

void VTKRender3DWidget::zoomView(double zoomFactor)
{
    vtkCamera *camera = m_impl->m_3DRenderer->GetActiveCamera();
    camera->Zoom(zoomFactor);
    ui->qvtkwidget->renderWindow()->Render();
}

void VTKRender3DWidget::setCameraOrientation(double pos[3], double focus[3], double up[3])
{
    // 设置新的位置、焦点和视上向量
    vtkCamera *camera = m_impl->m_3DRenderer->GetActiveCamera();

    camera->SetPosition(pos);
    camera->SetFocalPoint(focus);
    camera->SetViewUp(up);

    m_impl->m_3DRenderer->ResetCameraClippingRange();
    ui->qvtkwidget->renderWindow()->Render();
}

vtkRenderWindow *VTKRender3DWidget::renderWindow() const
{
    if (ui->qvtkwidget) return ui->qvtkwidget->renderWindow();
    return nullptr;
}

QVTKInteractor *VTKRender3DWidget::interactor() const
{
    if (ui->qvtkwidget)
        return ui->qvtkwidget->interactor();
    else
        return nullptr;
}

QVTKOpenGLNativeWidget *VTKRender3DWidget::getVTKWidget() const
{
    return ui->qvtkwidget;
}

void VTKRender3DWidget::addActor(std::string id, vtkProp *actor)
{
    auto it = m_actorMap.find(id);
    if (it != m_actorMap.end()) {
        return;
    }
    m_actorMap.insert(std::make_pair(id, actor));
    m_impl->m_3DRenderer->AddActor(actor);

    ui->qvtkwidget->renderWindow()->Render();
}

void VTKRender3DWidget::vtkRenderUpdate()
{
    ui->qvtkwidget->renderWindow()->Render();
}

vtkRenderer *VTKRender3DWidget::renderer3D()
{
    return m_impl->m_3DRenderer;
}

bool VTKRender3DWidget::hasActor(const std::string &id)
{
    auto it = m_actorMap.find(id);
    if (it != m_actorMap.end()) {
        return true;
    }

    return false;
}

void VTKRender3DWidget::removeActor(const std::string &id)
{
    auto it = m_actorMap.find(id);
    m_impl->m_3DRenderer->RemoveActor(it->second);
    m_actorMap.erase(id);
}

void VTKRender3DWidget::removeAllActors()
{
    auto it = m_actorMap.begin();

    for (; it != m_actorMap.end(); it++) {
        m_impl->m_3DRenderer->RemoveActor(it->second);
    }
    m_actorMap.clear();
}
vtkProp*VTKRender3DWidget::getActor(std::string id)
{
    auto it = m_actorMap.find(id);
    if (it != m_actorMap.end()) {
        return it->second;
    }

    return nullptr;
}

void VTKRender3DWidget::initPatientInfo()
{
}

void VTKRender3DWidget::setPatientInfoVisible(bool visible)
{
    ui->m_patientInfoPanel->setVisible(visible);
}

void VTKRender3DWidget::setDirectCubeVisible(bool visible)
{
    m_impl->m_axes->SetVisibility(visible);
}
