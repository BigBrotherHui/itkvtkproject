/**
 * @File Name: VTKResliceCursorCallback.h
 * @brief  二维视图上十字光标交互回调类，支持的交互操作有
 *         1、按住鼠标左键后移动鼠标，其他二维视图切片窗层数协同更新
 * @Author : wangxiaofeng
 * @Version : 1.0
 * @Creat Date : 2023-06-09
 *
 */
#pragma once

#include <vtkCommand.h>
#include <vtkImagePlaneWidget.h>
#include <vtkResliceCursorWidget.h>
#include <vtkWidgetEvent.h>

#include "VTKRender2DWidget.h"

// 回调类，当十字光标修改时调用
class VTKResliceCursorCallback : public vtkCommand {
public:
    static VTKResliceCursorCallback *New() { return new VTKResliceCursorCallback; }

    void setMPRXWidget(VTKRender2DWidget *pwidget);
    void setMPRYWidget(VTKRender2DWidget *pwidget);
    void setMPRZWidget(VTKRender2DWidget *pwidget);

    // 重写虚函数
    virtual void Execute(vtkObject *caller, unsigned long eventId, void *callData) override;

    VTKResliceCursorCallback();
    virtual ~VTKResliceCursorCallback();

    // vtkImagePlaneWidget *IPW[3];
    // vtkResliceCursorWidget *m_RCW[3];

    VTKRender2DWidget *m_render2DWidget[3] = {nullptr, nullptr, nullptr};

    // int clickNum = 0;
};
