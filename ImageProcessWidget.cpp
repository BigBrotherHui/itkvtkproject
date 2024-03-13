#include "ImageProcessWidget.h"
#include "ui_ImageProcessWidget.h"
#include <QMessageBox>
#include <opencv2/opencv.hpp>
#include <QtConcurrent/QtConcurrent>
#include <vtkDiscreteFlyingEdges3D.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkVTKImageToImageFilter.h>
#include <itkImageToVTKImageFilter.h>
#include <itkSmoothingRecursiveGaussianImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <vtkImageMathematics.h>
ImageProcessWidget::ImageProcessWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::ImageProcessWidget)
{
    ui->setupUi(this);
    ui->spinBox_threshold->setValue(20);
    ui->checkBox_volumeVisible->setChecked(true);
    connect(ui->checkBox_volumeVisible, &QCheckBox::stateChanged, this, &ImageProcessWidget::signal_volumeVisible);
}

ImageProcessWidget::~ImageProcessWidget()
{
    delete ui;
}

void ImageProcessWidget::setImageData(vtkSmartPointer<vtkImageData> imageData)
{
    if (!imageData || m_imageData==imageData)
       return;
    m_imageData = imageData;
}

vtkSmartPointer<vtkPolyData> ImageProcessWidget::GetPolyDataResult()
{
    return m_polyData;
}

vtkSmartPointer<vtkImageData> ImageProcessWidget::GetImageMask()
{
    return m_imageMask;
}

void ImageProcessWidget::closeEvent(QCloseEvent* event)
{
    hide();
}

void ImageProcessWidget::on_pushButton_threshold_clicked()
{
    if (!m_imageData)
        return;
    int _threshold = ui->spinBox_threshold->value();
    itk::VTKImageToImageFilter<ImageType>::Pointer importer = itk::VTKImageToImageFilter<ImageType>::New();
    importer->SetInput(m_imageData);
    importer->Update();
    ImageType::Pointer image = importer->GetOutput();
    using FilterType = itk::BinaryThresholdImageFilter<ImageType, ImageType>;
    FilterType::Pointer filter = FilterType::New();
    filter->SetInput(image);
    const PixelType outsidevalue = 0;
    const PixelType insidevalue = 1;
    filter->SetOutsideValue(outsidevalue);
    filter->SetInsideValue(insidevalue);
    const PixelType lowerThreshold = _threshold;
    const PixelType upperThreshold = 255;
    filter->SetUpperThreshold(upperThreshold);
    filter->SetLowerThreshold(lowerThreshold);
    try
    {
        filter->Update();// Running Filter;
    }
    catch (itk::ExceptionObject& e)
    {
        cout << "Caught Error!" << endl;
        cout << e.what() << endl;
        return;
    }
    itk::ImageToVTKImageFilter<ImageType>::Pointer iim = itk::ImageToVTKImageFilter<ImageType>::New();
    iim->SetInput(filter->GetOutput());
    iim->Update();
    if(!m_imageMask)
    {
        m_imageMask = vtkSmartPointer<vtkImageData>::New();
    }
    m_imageMask->DeepCopy(iim->GetOutput());
    /*QFuture<void> future = QtConcurrent::run(this, &ImageProcessWidget::threshold, m_imageData, _threshold, 255);
    future.waitForFinished();*/
    emit signal_operationFinished();
}

void ImageProcessWidget::on_pushButton_blur_clicked()
{
    itk::VTKImageToImageFilter<ImageType>::Pointer importer = itk::VTKImageToImageFilter<ImageType>::New();
    importer->SetInput(m_imageData);
    importer->Update();
    ImageType::Pointer image = importer->GetOutput();
    using FilterType = itk::SmoothingRecursiveGaussianImageFilter<ImageType, ImageType>;
    FilterType::Pointer smoothFilter = FilterType::New();
    const float sigmaValue = 3;
    smoothFilter->SetSigma(sigmaValue);
    smoothFilter->SetInput(image);
    smoothFilter->Update();
    itk::ImageToVTKImageFilter<ImageType>::Pointer iim = itk::ImageToVTKImageFilter<ImageType>::New();
    iim->SetInput(smoothFilter->GetOutput());
    iim->Update();
    m_imageData->DeepCopy(iim->GetOutput());
    /*QFuture<void> future=QtConcurrent::run(this,&ImageProcessWidget::blur,m_imageData,11,11);
    future.waitForFinished();*/
    emit signal_operationFinished();
}

void ImageProcessWidget::on_pushButton_marchingcubes_clicked()
{
    marchingCubesConstruction(m_imageData);
    emit signal_operationFinished();
}

void ImageProcessWidget::on_pushButton_smooth_clicked()
{
    smooth(m_polyData,20);
    emit signal_operationFinished();
}
#include "itkSimpleContourExtractorImageFilter.h"
#include <itkBinaryImageToStatisticsLabelMapFilter.h>
void ImageProcessWidget::on_pushButton_measure_clicked() {
	itk::VTKImageToImageFilter<ImageType>::Pointer importer = itk::VTKImageToImageFilter<ImageType>::New();
	importer->SetInput(m_imageMask);
	importer->Update();
	itk::VTKImageToImageFilter<ImageType>::Pointer importer2 = itk::VTKImageToImageFilter<ImageType>::New();
	importer2->SetInput(m_imageData);
	importer2->Update();
	typedef itk::BinaryImageToStatisticsLabelMapFilter< ImageType, ImageType > ConverterType;
	ConverterType::Pointer converter = ConverterType::New();
	converter->SetInput(importer->GetOutput());
	converter->SetFeatureImage(importer2->GetOutput());
	converter->SetFullyConnected(false);
	converter->Update();
	auto labels=converter->GetOutput()->GetLabels();
	for (auto it = labels.begin(); it != labels.end(); ++it) {
		auto labelObject = converter->GetOutput()->GetLabelObject(*it);
        ui->label_PhysicalSize->setText(QString::number(labelObject->GetPhysicalSize())+"立方毫米");
	}
}
void ImageProcessWidget::on_pushButton_canny_clicked()
{
    itk::VTKImageToImageFilter<ImageType>::Pointer importer = itk::VTKImageToImageFilter<ImageType>::New();
    importer->SetInput(m_imageMask);
    importer->Update();
    ImageType::Pointer image = importer->GetOutput();
    using SimpleContourExtractorImageFilterType = itk::SimpleContourExtractorImageFilter<ImageType, ImageType>;
    SimpleContourExtractorImageFilterType::Pointer contourFilter
        = SimpleContourExtractorImageFilterType::New();
    contourFilter->SetInput(image);
    contourFilter->Update();

    itk::ImageToVTKImageFilter<ImageType>::Pointer iim = itk::ImageToVTKImageFilter<ImageType>::New();
    iim->SetInput(contourFilter->GetOutput());
    iim->Update();
	if (!m_imageMask)
	{
		m_imageMask = vtkSmartPointer<vtkImageData>::New();
	}
	m_imageMask->DeepCopy(iim->GetOutput());
    /*QFuture<void> future = QtConcurrent::run(this, &ImageProcessWidget::canny, m_imageData, 0, 255);
    future.waitForFinished();*/
    emit signal_operationFinished();
}
#include "itkConnectedComponentImageFilter.h"
#include "itkLabelShapeKeepNObjectsImageFilter.h"

void ImageProcessWidget::on_pushButton_connectedComponentsWithStats_clicked()
{
    if(!m_imageMask)
        return;
    vtkSmartPointer<vtkImageMathematics> add = vtkSmartPointer<vtkImageMathematics>::New();
    add->SetInput1Data(m_imageMask);
    add->SetInput2Data(m_imageData);
    add->SetOperationToMultiply();
    add->Update();
    itk::VTKImageToImageFilter<ImageType>::Pointer importer = itk::VTKImageToImageFilter<ImageType>::New();
    importer->SetInput(add->GetOutput());
    importer->Update();
    ImageType::Pointer image = importer->GetOutput();
    using ConnectedComponentFilter = itk::ConnectedComponentImageFilter<ImageType, ImageType>;
    auto connnectedComponentFilter = ConnectedComponentFilter::New();
    connnectedComponentFilter->SetInput(image);
    connnectedComponentFilter->Update();
    using LabelShapeKeepNObjectsFilter = itk::LabelShapeKeepNObjectsImageFilter<ImageType>;
    auto labelShapeKeepNObjectsFilter = LabelShapeKeepNObjectsFilter::New();
    labelShapeKeepNObjectsFilter->SetInput(connnectedComponentFilter->GetOutput());
    labelShapeKeepNObjectsFilter->SetBackgroundValue(0);
    labelShapeKeepNObjectsFilter->SetNumberOfObjects(1);
    labelShapeKeepNObjectsFilter->SetAttribute(
        LabelShapeKeepNObjectsFilter::LabelObjectType::NUMBER_OF_PIXELS);
    labelShapeKeepNObjectsFilter->Update();
    typedef itk::RescaleIntensityImageFilter<ImageType, ImageType> RescaleFilterType;
    RescaleFilterType::Pointer rescaleFilter = RescaleFilterType::New();
    rescaleFilter->SetOutputMinimum(0);
    rescaleFilter->SetOutputMaximum(255);
    rescaleFilter->SetInput(labelShapeKeepNObjectsFilter->GetOutput());
    rescaleFilter->Update();
    itk::ImageToVTKImageFilter<ImageType>::Pointer iim = itk::ImageToVTKImageFilter<ImageType>::New();
    iim->SetInput(rescaleFilter->GetOutput());
    iim->Update();
    if (!m_imageMask)
    {
        m_imageMask = vtkSmartPointer<vtkImageData>::New();
    }
    m_imageMask->DeepCopy(iim->GetOutput());
    //QFuture<void> future = QtConcurrent::run(this, &ImageProcessWidget::connectedComponentsWithStats,m_imageData);
    //future.waitForFinished();
    emit signal_operationFinished();
}

void ImageProcessWidget::blur(vtkSmartPointer<vtkImageData> image, int blurX, int blurY)
{
    if (!image)
        return;
    int* dims = image->GetDimensions();
    cv::Mat cvMat(dims[0], dims[1], CV_8UC1);
#pragma omp parallel for
    for (int i = 0; i < dims[2]; ++i)
    {
        for (int j = 0; j < dims[1]; ++j)
        {
            for (int k = 0; k < dims[0]; ++k)
            {
                uchar* pPixel0 = (uchar*)(image->GetScalarPointer(k, j, i));
                cvMat.at<uchar>(cv::Point(j, k)) = *pPixel0;
            }
        }
        cv::GaussianBlur(cvMat, cvMat, cv::Size(blurX, blurY), 0);
        for (int m = 0; m < dims[1]; ++m)
        {
            for (int n = 0; n < dims[0]; ++n)
            {
                uchar* pPixel = (uchar*)(image->GetScalarPointer(n, m, i));
                *pPixel = cvMat.at<uchar>(cv::Point(m, n));
            }
        }
    }
    image->Modified();
}

void ImageProcessWidget::threshold(vtkSmartPointer<vtkImageData> image, int param_threshold, int maxval)
{
    if (!image)
        return;
    int* dims = image->GetDimensions();
    cv::Mat cvMat(dims[0], dims[1], CV_8UC1);
#pragma omp parallel for
    for (int i = 0; i < dims[2]; ++i)
    {
        for (int j = 0; j < dims[1]; ++j)
        {
            for (int k = 0; k < dims[0]; ++k)
            {
                uchar* pPixel0 = (uchar*)(image->GetScalarPointer(k, j, i));
                cvMat.at<uchar>(cv::Point(j, k)) = *pPixel0;
            }
        }
        cv::threshold(cvMat, cvMat, param_threshold,maxval,cv::THRESH_BINARY);
        for (int m = 0; m < dims[1]; ++m)
        {
            for (int n = 0; n < dims[0]; ++n)
            {
                uchar* pPixel = (uchar*)(image->GetScalarPointer(n, m, i));
                *pPixel = cvMat.at<uchar>(cv::Point(m, n));
            }
        }
    }
    image->Modified();
}

void ImageProcessWidget::canny(vtkSmartPointer<vtkImageData> image,int threshold1,int threshold2)
{
    if (!image)
        return;
    int* dims = image->GetDimensions();
    cv::Mat cvMat(dims[0], dims[1], CV_8UC1);
#pragma omp parallel for
    for (int i = 0; i < dims[2]; ++i)
    {
        for (int j = 0; j < dims[1]; ++j)
        {
            for (int k = 0; k < dims[0]; ++k)
            {
                uchar* pPixel0 = (uchar*)(image->GetScalarPointer(k, j, i));
                cvMat.at<uchar>(cv::Point(j, k)) = *pPixel0;
            }
        }
        cv::Canny(cvMat, cvMat, threshold1, threshold2);
        for (int m = 0; m < dims[1]; ++m)
        {
            for (int n = 0; n < dims[0]; ++n)
            {
                uchar* pPixel = (uchar*)(image->GetScalarPointer(n, m, i));
                *pPixel = cvMat.at<uchar>(cv::Point(m, n));
            }
        }
    }
    image->Modified();
}

void ImageProcessWidget::connectedComponentsWithStats(vtkSmartPointer<vtkImageData> image)
{
    if (!image)
        return;
    int* dims = image->GetDimensions();
    cv::Mat cvMat(dims[0], dims[1], CV_8UC1);
#pragma omp parallel for
    for (int i = 0; i < dims[2]; ++i)
    {
        for (int j = 0; j < dims[1]; ++j)
        {
            for (int k = 0; k < dims[0]; ++k)
            {
                uchar* pPixel0 = (uchar*)(image->GetScalarPointer(k, j, i));
                cvMat.at<uchar>(cv::Point(j, k)) = *pPixel0;
            }
        }
        cv::Mat labels, stats, centroids;
        int nccomps = cv::connectedComponentsWithStats(cvMat,labels,stats,centroids);
        std::vector<int> histogram_of_labels;
        for (int i = 0; i < nccomps; i++)//初始化labels的个数为0
        {
            histogram_of_labels.push_back(0);
        }

        int rows = labels.rows;
        int cols = labels.cols;
        for (int row = 0; row < rows; row++) //计算每个labels的个数
        {
            for (int col = 0; col < cols; col++)
            {
                histogram_of_labels.at(labels.at<unsigned short>(row, col)) += 1;
            }
        }
        histogram_of_labels.at(0) = 0; //将背景的labels个数设置为0

        //2. 计算最大的连通域labels索引
        int maximum = 0;
        int max_idx = 0;
        for (int i = 0; i < nccomps; i++)
        {
            if (histogram_of_labels.at(i) > maximum)
            {
                maximum = histogram_of_labels.at(i);
                max_idx = i;
            }
        }

        //3. 将最大连通域标记为1
        for (int row = 0; row < rows; row++)
        {
            for (int col = 0; col < cols; col++)
            {
                if (labels.at<unsigned short>(row, col) == max_idx)
                {
                    labels.at<unsigned short>(row, col) = 255;
                }
                else
                {
                    labels.at<unsigned short>(row, col) = 0;
                }
            }
        }
        labels.convertTo(cvMat, CV_8UC1);
    	for (int m = 0; m < dims[1]; ++m)
        {
            for (int n = 0; n < dims[0]; ++n)
            {
                uchar* pPixel = (uchar*)(image->GetScalarPointer(n, m, i));
                *pPixel = cvMat.at<uchar>(cv::Point(m, n));
            }
        }
    }
    image->Modified();
}
#include <QColorDialog>
void ImageProcessWidget::on_pushButton_setColor_clicked(){
    QColor color=QColorDialog::getColor(Qt::green, this, "请选择面绘制的颜色");
    if (color.isValid()) {
        emit signal_switchRenderColor(color);
    }
}
#include <vtkMassProperties.h>
#include <vtkTriangleFilter.h>
void ImageProcessWidget::marchingCubesConstruction(vtkSmartPointer<vtkImageData> image)
{
    if (!image)
        return;
	vtkSmartPointer<vtkImageMathematics> add = vtkSmartPointer<vtkImageMathematics>::New();
	add->SetInput1Data(m_imageMask);
	add->SetInput2Data(m_imageData);
	add->SetOperationToMultiply();
	add->Update();
    vtkSmartPointer<vtkDiscreteFlyingEdges3D> flying= vtkSmartPointer<vtkDiscreteFlyingEdges3D>::New();
    flying->SetInputData(add->GetOutput());
    flying->GenerateValues(20, m_imageData->GetScalarRange());
    flying->SetComputeGradients(false);
    flying->SetComputeNormals(false);
    flying->SetComputeScalars(false);
    flying->Update();
    
    if (!m_polyData)
        m_polyData = vtkSmartPointer<vtkPolyData>::New();
    m_polyData->DeepCopy(flying->GetOutput());
}

void ImageProcessWidget::smooth(vtkSmartPointer<vtkPolyData> po, int iteraterConunt)
{
    if(!po)
        return;
    vtkSmartPointer<vtkWindowedSincPolyDataFilter> smt = vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
    smt->SetInputData(po);
    smt->SetNumberOfIterations(iteraterConunt);
    smt->Update();
    if (!m_polyData)
        m_polyData = vtkSmartPointer<vtkPolyData>::New();
    m_polyData->DeepCopy(smt->GetOutput());
}
