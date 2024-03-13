#ifndef IMAGEPROCESSWIDGET_H
#define IMAGEPROCESSWIDGET_H

#include <QDockWidget>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <itkImage.h>

namespace Ui {
class ImageProcessWidget;
}

class ImageProcessWidget : public QDockWidget
{
    Q_OBJECT
    using PixelType = unsigned char;
    using ImageType = itk::Image<PixelType, 3>;
public:
    explicit ImageProcessWidget(QWidget *parent = nullptr);
    ~ImageProcessWidget();
    void setImageData(vtkSmartPointer<vtkImageData> imageData);
    vtkSmartPointer<vtkPolyData> GetPolyDataResult();
    vtkSmartPointer<vtkImageData> GetImageMask();
signals:
    void signal_operationFinished();
    void signal_volumeVisible(int);
protected slots:
    void on_pushButton_threshold_clicked();
    void on_pushButton_blur_clicked();
    void on_pushButton_marchingcubes_clicked();
    void on_pushButton_smooth_clicked();
    void on_pushButton_canny_clicked();
    void on_pushButton_connectedComponentsWithStats_clicked();
protected:
    void closeEvent(QCloseEvent* event) override;
    void blur(vtkSmartPointer<vtkImageData> image,int blurX,int blurY);
    void threshold(vtkSmartPointer<vtkImageData> image, int param_threshold, int maxval);
    void canny(vtkSmartPointer<vtkImageData> image, int threshold1, int threshold2);
    void connectedComponentsWithStats(vtkSmartPointer<vtkImageData> image);
    void marchingCubesConstruction(vtkSmartPointer<vtkImageData> image);
    void smooth(vtkSmartPointer<vtkPolyData> po,int iteraterConunt);
private:
    Ui::ImageProcessWidget *ui;
    vtkSmartPointer<vtkImageData> m_imageData{ nullptr };
    vtkSmartPointer<vtkPolyData> m_polyData{ nullptr };
    vtkSmartPointer<vtkImageData> m_imageMask{nullptr};
};

#endif // IMAGEPROCESSWIDGET_H
