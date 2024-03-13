#include "VTKRender2DWidget.h"

#include <vtkActor2D.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkCornerAnnotation.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLine.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkResliceCursorActor.h>
#include "ResliceCursorLineRepresentation.h"
#include <vtkResliceCursorPolyDataAlgorithm.h>
#include <vtkResliceCursorThickLineRepresentation.h>
#include <vtkResliceCursorWidget.h>
#include <vtkResliceImageViewer.h>
#include <vtkTextProperty.h>

#include <QGraphicsDropShadowEffect>
#include <QPainter>

#include "VTKResliceImageViewer2.h"
#include "ui_VTKRender2DWidget.h"
#include <QDebug>
#include <QSlider>

class VTKRender2DWidgetImpl {
public:
    VTKRender2DWidgetImpl();

    ~VTKRender2DWidgetImpl() {}

    void initImageWindowLevel();

public:
    vtkNew<vtkResliceImageViewer2> m_pImageViewer;  // vtkResliceImageViewer，这个后续可能需要替换成其子类

    vtkImageData *m_ImageData{nullptr};             // 图像数据

    int m_iImageSlice;                              // 当前图像切片序列号
    int m_iImageOrient;        // 当前图像方向，1表示冠状面、0表示横断面、2表示矢状面

    int m_iImageWindow = 800;  // 当前图像的窗宽
    int m_iImageLevel = 1012;  // 当前图像的窗位

    int m_iniSliceIndex = 0;
    bool m_bScrollBarChangeValue = false;
    bool m_bCursorChangePosition = false;
};

VTKRender2DWidgetImpl::VTKRender2DWidgetImpl()
{
    m_ImageData = nullptr;
    m_iImageSlice = 0;
    m_iImageOrient = 1;
}

void VTKRender2DWidgetImpl::initImageWindowLevel()
{
    m_iImageWindow = 800;
    m_iImageLevel = 1012;
}

//--------------------------------------------------------------------------------------------
// VTKRendere2DWidget Class implementation
//
//--------------------------------------------------------------------------------------------

// constructor
VTKRender2DWidget::VTKRender2DWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::VTKRender2DWidget), m_impl(std::make_unique<VTKRender2DWidgetImpl>())
{
    ui->setupUi(this);

    ui->horizontalScrollBar->raise();
    ui->horizontalScrollBar->setRange(0, 0);
}

VTKRender2DWidget::~VTKRender2DWidget()
{
    delete ui;
}

void VTKRender2DWidget::setImageOrient(int orient)
{
    m_impl->m_iImageOrient = orient;
    m_impl->m_pImageViewer->SetSliceOrientation(2 - orient % 3);
    m_impl->m_pImageViewer->SetResliceModeToAxisAligned();
}

#include <vtkDICOMImageReader.h>

void VTKRender2DWidget::setImageData(vtkImageData *pImageData)
{
    if (pImageData == nullptr) return;

    m_impl->m_ImageData = pImageData;

    // 获取图像基本属性，如维度、尺寸等
    int imageDim[3], extent[6], iMaxRange = 0, iMinRange = 0;
    double spacing[3];  // 像素间的距离
    pImageData->GetDimensions(imageDim);
    pImageData->GetExtent(extent);
    pImageData->GetSpacing(spacing);
    iMaxRange = imageDim[2 - m_impl->m_iImageOrient];
    iMinRange = 0;  // 序列号从1开始

    m_impl->m_iniSliceIndex = m_impl->m_iImageSlice = (iMinRange + iMaxRange) / 2;

    // 关联渲染窗口与交互器
    m_impl->m_pImageViewer->SetInputData(pImageData);
    m_impl->m_pImageViewer->SetRenderWindow(ui->openGLWidget->renderWindow());
    m_impl->m_pImageViewer->SetupInteractor(ui->openGLWidget->interactor());
    m_impl->m_pImageViewer->SetSlice(m_impl->m_iImageSlice);

    // 设置十字光标
    ResliceCursorLineRepresentation *rep = ResliceCursorLineRepresentation::SafeDownCast(
        m_impl->m_pImageViewer->GetResliceCursorWidget()->GetRepresentation());
    rep->GetResliceCursorActor()->GetCursorAlgorithm()->SetReslicePlaneNormal(2 - m_impl->m_iImageOrient % 3);
    rep->GetResliceCursorActor()->GetCenterlineProperty(0)->SetRepresentationToWireframe();
    rep->GetResliceCursorActor()->GetCenterlineProperty(1)->SetRepresentationToWireframe();
    rep->GetResliceCursorActor()->GetCenterlineProperty(2)->SetRepresentationToWireframe();
    m_impl->m_pImageViewer->SetResliceMode(1);
    // m_impl->m_pImageViewer->SetResliceMode(0);

    // 设置scrollbar属性参数
    ui->horizontalScrollBar->setRange(iMinRange, iMaxRange - 1);
    ui->horizontalScrollBar->setValue(m_impl->m_iImageSlice);
    ui->horizontalScrollBar->setToolTip(QString::number(m_impl->m_iImageSlice));

    QString text = QString::number(m_impl->m_iImageSlice) + "/" + QString::number(iMaxRange);
    ui->m_sliceIndexLb->setText(text);

    // 更新渲染场景
    m_impl->m_pImageViewer->GetRenderer()->ResetCamera();
    m_impl->m_pImageViewer->Render();
    ui->openGLWidget->renderWindow()->Render();

    // 构建信号槽
    disconnect(ui->horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(updateCursor(int)));
    connect(ui->horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(updateCursor(int)));
}

void VTKRender2DWidget::renderAll()
{
    m_impl->m_pImageViewer->Render();
    // m_impl->m_pImageViewer->GetRenderer()->ResetCamera();
    ui->openGLWidget->renderWindow()->Render();
}

void VTKRender2DWidget::initImageWindowLevel()
{
    m_impl->initImageWindowLevel();
}

void VTKRender2DWidget::setWindowLevel(int window, int level)
{
    if (m_impl->m_ImageData) {
        m_impl->m_pImageViewer->SetColorWindow(window);
        m_impl->m_pImageViewer->SetColorLevel(level);

        m_impl->m_pImageViewer->Render();
    }
}

void VTKRender2DWidget::setImageSlice(int sliceIndex)
{
    int oldSlice = this->getImageSlice();
    if (oldSlice != sliceIndex) {
        this->updateImageSlice(sliceIndex);
        emit signalImageSliceChanged(sliceIndex);
    }
}

int VTKRender2DWidget::getImageSlice() const
{
    return m_impl->m_pImageViewer->GetSlice();
}

int VTKRender2DWidget::getImageOrient() const
{
    return m_impl->m_iImageOrient;
}

vtkResliceCursor *VTKRender2DWidget::getResliceCursor() const
{
    return m_impl->m_pImageViewer->GetResliceCursor();
}

void VTKRender2DWidget::setResliceCursor(vtkResliceCursor *cursor)
{
    if (cursor == nullptr) return;

    // 设置curosr的参数

    m_impl->m_pImageViewer->SetResliceCursor(cursor);
}

vtkResliceCursorWidget *VTKRender2DWidget::getResliceCursorWidget() const
{
    return m_impl->m_pImageViewer->GetResliceCursorWidget();
}

void VTKRender2DWidget::setResliceMode(int reslicemode)
{
    if (m_impl) {
        m_impl->m_pImageViewer->SetResliceMode(reslicemode);
    }
}

void VTKRender2DWidget::resetResliceCursor()
{
    if (m_impl->m_ImageData) {
        m_impl->m_pImageViewer->Reset();
        m_impl->m_pImageViewer->Render();

        ui->horizontalScrollBar->setValue(m_impl->m_iniSliceIndex);
    }
}

void VTKRender2DWidget::UpdateSliceCursorPosition(double x, double y, double z)
{
    if (m_impl->m_ImageData == nullptr) return;

    // 更新当前二维视图中拾取点所在的图像切片位置
    this->setSliceCursorPosition(x, y, z);
    this->renderAll();
}

void VTKRender2DWidget::setSliceCursorPosition(double x, double y, double z)
{
    if (m_impl->m_ImageData == nullptr) return;

    double worldPoint[3] = {x, y, z};
    double imagePoint[3] = {0, 0, 0};

    m_impl->m_ImageData->TransformPhysicalPointToContinuousIndex(worldPoint, imagePoint);
}

void VTKRender2DWidget::setSliceCursorPosition(int iX, int iY, int iZ)
{
    if (m_impl->m_ImageData == nullptr) return;

    int extent[6];
    m_impl->m_ImageData->GetExtent(extent);

    iX = iX < extent[0] ? extent[0] : iX;
    iX = iX > extent[1] ? extent[1] : iX;
    iY = iY < extent[2] ? extent[2] : iY;
    iY = iY > extent[3] ? extent[3] : iY;
    iZ = iZ < extent[4] ? extent[4] : iZ;
    iZ = iZ > extent[5] ? extent[5] : iZ;

    // 根据图像切片方向来更新显示图像层数
    switch (m_impl->m_iImageOrient) {
        case 0:  // xy平面
        {
            this->updateImageSlice(iZ);
            break;
        }
        case 1:  // xz平面
        {
            this->updateImageSlice(iY);
            break;
        }
        case 2:  // yz平面
        {
            this->updateImageSlice(iZ);
            break;
        }
    }
}

void VTKRender2DWidget::showCornerAnnotation(bool show)
{
}

void VTKRender2DWidget::setBackGroundColor(int r, int g, int b, int a)
{
    // m_BackGroundColor.setRgb(r, g, b, a);
    m_BackGroundColor.setRgb(0, 0, 0, 0);
    this->update();
}

QVTKOpenGLNativeWidget *VTKRender2DWidget::getVTKWidget() const
{
    return ui->openGLWidget;
}

vtkRenderWindow *VTKRender2DWidget::renderWindow() const
{
    if (ui->openGLWidget) return ui->openGLWidget->renderWindow();
    return nullptr;
}

QVTKInteractor *VTKRender2DWidget::interactor() const
{
    if (ui->openGLWidget)
        return ui->openGLWidget->interactor();
    else
        return nullptr;
}

vtkRenderer *VTKRender2DWidget::getRenderer() const
{
    return m_impl->m_pImageViewer->GetRenderer();
}

vtkResliceImageViewer2 *VTKRender2DWidget::getResliceImageViewer2() const
{
    return m_impl->m_pImageViewer.GetPointer();
}

void VTKRender2DWidget::setRenderWindow(vtkGenericOpenGLRenderWindow *win)
{
    if (ui->openGLWidget) ui->openGLWidget->setRenderWindow(win);
}

void VTKRender2DWidget::setRenderWindow(vtkRenderWindow *win)
{
    if (ui->openGLWidget) ui->openGLWidget->setRenderWindow(win);
}

void VTKRender2DWidget::mouseMoveEvent(QMouseEvent *event)
{
    m_bIsMouseMove = true;
}
//------------------------------------------------------------------------------------------
// 槽函数
//------------------------------------------------------------------------------------------
void VTKRender2DWidget::updateCursor(int value)
{
    if (m_impl->m_bCursorChangePosition) {
        m_impl->m_bCursorChangePosition = false;
        return;
    }
    m_impl->m_bScrollBarChangeValue = true;
    int incValue = value - m_impl->m_iImageSlice;
    m_impl->m_pImageViewer->IncrementSlice(incValue);
    m_impl->m_iImageSlice = m_impl->m_iImageSlice + incValue;

    int *sliceRange = m_impl->m_pImageViewer->GetSliceRange();
    QString text = QString::number(m_impl->m_iImageSlice) + "/" + QString::number(sliceRange[1]);
    ui->m_sliceIndexLb->setText(text);
}

void VTKRender2DWidget::updateImageSlice(int value)
{
    if (m_impl->m_bScrollBarChangeValue) {
        m_impl->m_bScrollBarChangeValue = false;
        return;
    }

    m_impl->m_bCursorChangePosition = true;
    // LOG_DEBUG("VTKRender2DWidget::updateImageSlice()");
    // LOG_DEBUG("value is {}", value + 1);

    // m_impl->m_iImageSlice = value + 1;
    m_impl->m_iImageSlice = value;
    m_impl->m_pImageViewer->SetSlice(m_impl->m_iImageSlice);

    // 有图像数据时才能显示角落文本注释
    if (m_impl->m_ImageData) {
        // 更新角落文本注释
        int *sliceRange = m_impl->m_pImageViewer->GetSliceRange();
        QString text = QString::number(m_impl->m_iImageSlice) + "/" + QString::number(sliceRange[1]);
        ui->m_sliceIndexLb->setText(text);
    }
    ui->horizontalScrollBar->setValue(value);
    ui->openGLWidget->renderWindow()->Render();
}
void VTKRender2DWidget::updateImageSliceToPickedCursor()
{
    m_impl->m_pImageViewer->SetSlice(m_impl->m_iImageSlice + 1);
    m_impl->m_pImageViewer->SetSlice(m_impl->m_iImageSlice - 1);
    ui->openGLWidget->renderWindow()->Render();
}

void VTKRender2DWidget::setSlice(int s)
{
    ui->horizontalScrollBar->setValue(s);
}

int VTKRender2DWidget::getSlice()
{
    return ui->horizontalScrollBar->value();
}

void VTKRender2DWidget::Dolly(double factor)
{
    vtkCamera *camera = this->getRenderer()->GetActiveCamera();
    // std::cout << camera->GetParallelScale() << endl;
    if (camera->GetParallelProjection()) {
        camera->SetParallelScale(factor);
    }
    else {
        camera->Dolly(factor);
    }

    if (this->getRenderer()->GetLightFollowCamera()) {
        this->getRenderer()->UpdateLightsGeometryToFollowCamera();
    }

    // this->getRenderer()->Render();
}
