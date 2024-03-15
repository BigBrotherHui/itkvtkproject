#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include "VTKResliceCursorCallback.h"
class ImageProcessWidget;
class QProgressDialog;
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void RenderAllVtkRenderWindows();
protected:
   
protected slots:
    void on_pushButton_openFile_clicked();
    void on_pushButton_openDir_clicked();
    void on_comboBox_blendMode_currentIndexChanged(int index);
    void on_pushButton_reset_clicked();
    void on_pushButton_imageProcess_clicked();

    void slot_operationFinished();
    void slot_volumeVisible(int);
    void slot_setRenderColor(QColor);
protected:
    bool loadImagesFromDirectory(QString path, QProgressDialog* dialog);
    bool loadImagesFromSingleFile(QString path, QProgressDialog* dialog);
    QProgressDialog* createProgressDialog(QString title, QString prompt, int range);
    bool eventFilter(QObject* watched, QEvent* event);
    void changeLayout(QWidget*);
    void take_over();
    void setButtonsEnable(bool,std::vector<QWidget *>except=std::vector<QWidget *>());
    void setButtonsEnable(bool, QWidget*except=nullptr);
private:
    Ui::MainWindow *ui;


private slots:
private:
    vtkSmartPointer<vtkImageData> m_imagedata{nullptr}, m_imagedata2d{ nullptr };
    vtkSmartPointer<VTKResliceCursorCallback> m_cbk;  // 十字光标交互回调类
    bool m_ismax{ false };
    ImageProcessWidget* m_imageProcessWidget{nullptr};
    bool m_isSingleFile{ false };
};

#endif // MAINWINDOW_H
