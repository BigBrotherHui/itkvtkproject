/**
 * @File Name: VTKRender2DWidget.h
 * @brief  2D渲染窗口，主要用于显示医学影像数据
 * @Author : wangxiaofeng
 * @Version : 1.0
 * @Creat Date : 2023-06-06
 *
 */

#ifndef VTK_RENDER2DWIDGET_H
#define VTK_RENDER2DWIDGET_H


/***********Qt*******************/
#include <QVTKInteractor.h>
#include <QVtkOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkResliceCursor.h>

#include <QWidget>

#include "VTKResliceImageViewer2.h"

namespace Ui {
class VTKRender2DWidget;
}

class VTKRender2DWidget : public QWidget {
    Q_OBJECT

public:
    explicit VTKRender2DWidget(QWidget *parent = nullptr);
    ~VTKRender2DWidget();

    /**
     * @brief  在2D渲染窗口中显示图像数据
     * @param  pImageData: 传入的图像数据指针vtkImageData
     */
    void setImageData(vtkImageData *pImageData);

    /**
     * @brief 设置显示DICOM图像的切片，图像横断面、冠状面、矢状面的显示
     * @param  orient: 表示图像方向的参数，0表示横断面, 1表示冠状面，2表示矢状面，可以用枚举替换
     */
    void setImageOrient(int orient);

    // 初始化图像的窗宽窗位
    void InitialWindowLevel();

    /**
     * @brief  设置切片层序列号，如超出图像序列范围，设置边界值
     * @param  sliceIndex: 图形序列索引号，超出序列范围，则设置边界值
     */
    void setImageSlice(int sliceIndex);

    // 返回切片层序列号
    int getImageSlice() const;

    /**
     * @brief  获取二维渲染窗口中三维图像切片方向
     * @return int: 0表示横断面, 1表示冠状面，2表示矢状面
     */
    int getImageOrient() const;

    void setResliceMode(int reslicemode = 0);

    /**
     * @brief  返回当前2D窗口的切片光标类
     * @return vtkResliceCursor*: 切片光标类
     */
    vtkResliceCursor *getResliceCursor() const;

    /**
     * @brief  设置当前2D窗口的切片光标类
     * @param  cursor: vtkResliceCursor*: 切片光标类
     */
    void setResliceCursor(vtkResliceCursor *cursor);

    /**
     * @brief  重设十字光标位置
     */
    void resetResliceCursor();

    /**
     * @brief  返回当前2D窗口的十字光标切片交互部件
     * @return vtkResliceCursorWidget*:
     */
    vtkResliceCursorWidget *getResliceCursorWidget() const;

    // 刷新渲染场景
    void renderAll();

    /**
     * @brief  鼠标单击选择点的位置， 需要发出信号通知MPRWidget，更新关联二维视图中切片位置
     * @param  x,y,z: 鼠标单击选择点的位置
     */
    void UpdateSliceCursorPosition(double x, double y, double z);

    /**
     * @brief  设置十字光标位置，该点是世界坐标系下的位置
     * @param  x,y,z: 在世界坐标系下的点位置
     */
    void setSliceCursorPosition(double x, double y, double z);

    //
    /**
     * @brief  设置十字光标位置，该位置是图像索引坐标系下的位置。同时更新ScrollBar、Text等信息
     * @param  x,y,z:
     */
    void setSliceCursorPosition(int x, int y, int z);

    /**
     * @brief  设置CT窗宽与窗位
     * @param  window:
     * @param  level:
     */
    void setWindowLevel(int window, int level);

    // 初始化图像的窗宽窗位
    void initImageWindowLevel();

    // 显示或隐藏窗口四个角落文本注释
    void showCornerAnnotation(bool show = true);

    /**
     * @brief  Set the Back Ground Color object
     */
    void setBackGroundColor(int r = 0, int g = 0, int b = 0, int a = 255);

    //------------------------------------------------------------------------------------
    QVTKOpenGLNativeWidget *getVTKWidget() const;
    vtkRenderWindow *renderWindow() const;
    QVTKInteractor *interactor() const;
    vtkRenderer *getRenderer() const;
    vtkResliceImageViewer2 *getResliceImageViewer2() const;

    void setRenderWindow(vtkGenericOpenGLRenderWindow *win);
    void setRenderWindow(vtkRenderWindow *win);

    void updateImageSliceToPickedCursor();
    void Dolly(double factor);
    void setPatientInfoVisible(bool visible);
    void setSlice(int s);
    int getSlice();
signals:

    /**
     * @brief  鼠标单击选择点的位置的信号
     * @param  x,y,z：鼠标单击选择点的位置
     * @param  orient: 二维视图切片方向，0/1/2
     */
    void signalPickedCursorChanged(double x, double y, double z, int orient);

    // 当前层变化的信号
    void signalImageSliceChanged(int slice);

public slots:
    void updateImageSlice(int value);
    void updateCursor(int vlaue);

protected:

    virtual void mouseMoveEvent(QMouseEvent *event) override;

private:
    ///**
    // * @brief paintEvent
    // * 重写绘图时间
    // * @param event
    // */
    // void paintEvent(QPaintEvent *event);
    ///**
    // * @brief resizeEvent
    // * 重置窗口尺寸
    // * @param event
    // */
    // void resizeEvent(QResizeEvent *event);
    ///**
    // * @brief enterEvent
    // * 鼠标进入事件
    // * @param event
    // */
    // void enterEvent(QEvent *event);
    ///**
    // * @brief leaveEvent
    // * 鼠标离开事件
    // * @param event
    // */
    // void leaveEvent(QEvent *event);

    void setPatientInfo();

private:
    Ui::VTKRender2DWidget *ui;

    friend class VTKRender2DWidgetImpl;
    std::unique_ptr<VTKRender2DWidgetImpl> m_impl;

    // QT控件定义
    QColor m_BackGroundColor = QColor(255, 255, 0, 255);  // 背景颜色
public:
    bool m_bIsMouseMove = false;
};

#endif  // VTK_RENDER2DWIDGET_H
