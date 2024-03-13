/**
 * @File Name: VTKRender3DWidget.h
 * @brief  绘制3D可视化渲染窗口类，可以直接用来显示3D可视化创建
 * @Author : wangxiaofeng
 * @Version : 1.0
 * @Creat Date : 2023-06-06
 *
 */

#ifndef XJTPANEL3D_H
#define XJTPANEL3D_H

#include <QVTKInteractor.h>
#include <QVtkOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>

#include <QWidget>
#include <memory>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkSmartPointer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include "Common.h"
namespace Ui {
class VTKRender3DWidget;
}

class VTKRender3DWidget : public QWidget {
    Q_OBJECT

public:
    explicit VTKRender3DWidget(QWidget *parent = nullptr);
    ~VTKRender3DWidget();
    void setImageData(vtkImageData* pImageData);
    void setBlendMode(int);
    /**
 * @brief  3D场景视图中相机姿态调整，如正视图、侧视图、顶视图等
 * @param  viewPort3D: 枚举类，记录相机的6种姿态
 */
    void switchCameraView(ViewPort3D viewPort3D);
    /**
     * @brief  自适应场景大小，视图缩放到能看到所有场景对象的状态
     * @param  reset:
     */
    void zoomView(bool reset = false);
    void zoomView(double zoomFactor);

    /**
     * @brief  通过传入相机位置、焦点和视上来调整相机姿态
     * @param  pos: 相机位置
     * @param  focus: 相机焦点
     * @param  up: 相机视上
     */
    void setCameraOrientation(double pos[3], double focus[3], double up[3]);

    // 相机焦点位置调整、而距离不变

    QVTKOpenGLNativeWidget *getVTKWidget() const;
    vtkRenderWindow *renderWindow() const;
    QVTKInteractor *interactor() const;

    void addActor(std::string id, vtkProp*actor);
    vtkProp*getActor(std::string id);
    void vtkRenderUpdate();

    void setPatientInfoVisible(bool visible);
    void setDirectCubeVisible(bool visible);

    /**
     * @brief 病人信息初始化，初始化组件时调用;
     * 要求在加载案例数据后调用
     * @return void
     */
    void initPatientInfo();

    // added by lijie 2023/7/4
    vtkRenderer *renderer3D();

    bool hasActor(const std::string &id);
    void removeActor(const std::string &id);

    void removeAllActors();

private:
    void initialRender();

private:
    std::map<std::string, vtkSmartPointer<vtkProp>> m_actorMap;
    Ui::VTKRender3DWidget *ui;
    class VTKRender3DWidgetImpl;
    std::unique_ptr<VTKRender3DWidgetImpl> m_impl;
    vtkSmartPointer<vtkVolume> m_volume;
    vtkSmartPointer<vtkPiecewiseFunction> m_opacityFunc;
    vtkSmartPointer<vtkPiecewiseFunction> m_gradientOpacityFunc;
    vtkSmartPointer<vtkColorTransferFunction> m_colorFunc;
    vtkSmartPointer<vtkSmartVolumeMapper> m_volMapper;
    vtkSmartPointer<vtkVolumeProperty> m_volProps;
};

#endif  // XJTPANEL3D_H
