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
#include <vtkInteractorStyleImage.h>
#include <QFileDialog>
#include <vtkSTLWriter.h>
#include <itkTIFFImageIO.h>
#include <itkImageSeriesWriter.h>
#include <itkNumericSeriesFileNames.h>
ImageProcessWidget::ImageProcessWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::ImageProcessWidget)
{
    ui->setupUi(this);
    ui->spinBox_threshold->setValue(20);
    ui->spinBox_threshold_2d->setValue(20);
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
    if (!m_imageMask2d)
    {
        m_imageMask2d = vtkSmartPointer<vtkImageData>::New();
    }
    if (m_imageData && ui->tabWidget==ui->tab_2DImageProcess)
    {
        m_imageMask2d->DeepCopy(m_imageData);
        m_imageactor->SetInputData(m_imageMask2d);
        m_renderwindow->Render();
    }
}

vtkSmartPointer<vtkPolyData> ImageProcessWidget::GetPolyDataResult()
{
    return m_polyData;
}

vtkSmartPointer<vtkImageData> ImageProcessWidget::GetImageMask()
{
    return m_imageMask;
}

void ImageProcessWidget::setCurrentPageTo2DImageProcess()
{
    if (!m_renderwindow)
    {
        m_renderwindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
        ui->widget_imagewindow->setRenderWindow(m_renderwindow);
        vtkSmartPointer<vtkInteractorStyleImage> style = vtkSmartPointer<vtkInteractorStyleImage>::New();
        ui->widget_imagewindow->interactor()->SetInteractorStyle(style);
    }
    if (!m_renderer)
    {
        m_renderer = vtkSmartPointer<vtkRenderer>::New();
        m_renderwindow->AddRenderer(m_renderer);
    }
    if(!m_imageactor)
    {
        m_imageactor = vtkSmartPointer<vtkImageActor>::New();
        m_renderer->AddActor(m_imageactor);
    }
    if(m_imageData)
    {
        m_imageactor->SetInputData(m_imageData);
    }
    ui->tabWidget->setCurrentWidget(ui->tab_2DImageProcess);
}

void ImageProcessWidget::setCurrentPageTo3DImageProcess()
{
    ui->tabWidget->setCurrentWidget(ui->tab_3DImageProcess);
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
    using OutImageType = itk::Image<unsigned short, 3>;
    using ConnectedComponentFilter = itk::ConnectedComponentImageFilter<ImageType, OutImageType>;
    auto connnectedComponentFilter = ConnectedComponentFilter::New();
    connnectedComponentFilter->SetInput(image);
    try
    {
        connnectedComponentFilter->Update();
    }
    catch (itk::ExceptionObject &e)
    {
        std::cout << "error connectedComponentsWithStats:" << e.what() << std::endl;
        return;
    }
    using LabelShapeKeepNObjectsFilter = itk::LabelShapeKeepNObjectsImageFilter<OutImageType>;
    auto labelShapeKeepNObjectsFilter = LabelShapeKeepNObjectsFilter::New();
    labelShapeKeepNObjectsFilter->SetInput(connnectedComponentFilter->GetOutput());
    labelShapeKeepNObjectsFilter->SetBackgroundValue(0);
    labelShapeKeepNObjectsFilter->SetNumberOfObjects(1);
    labelShapeKeepNObjectsFilter->SetAttribute(
        LabelShapeKeepNObjectsFilter::LabelObjectType::NUMBER_OF_PIXELS);
    try
    {
        labelShapeKeepNObjectsFilter->Update();
    }
    catch (itk::ExceptionObject& e)
    {
        std::cout << "error labelShapeKeepNObjectsFilter:" << e.what() << std::endl;
        return;
    }
    typedef itk::RescaleIntensityImageFilter<OutImageType, ImageType> RescaleFilterType;
    RescaleFilterType::Pointer rescaleFilter = RescaleFilterType::New();
    rescaleFilter->SetOutputMinimum(0);
    rescaleFilter->SetOutputMaximum(255);
    rescaleFilter->SetInput(labelShapeKeepNObjectsFilter->GetOutput());
    rescaleFilter->Update();
    try
    {
        rescaleFilter->Update();
    }
    catch (itk::ExceptionObject& e)
    {
        std::cout << "error rescaleFilter:" << e.what() << std::endl;
        return;
    }
    itk::ImageToVTKImageFilter<ImageType>::Pointer iim = itk::ImageToVTKImageFilter<ImageType>::New();
    iim->SetInput(rescaleFilter->GetOutput());
    iim->Update();
    if (!m_imageMask)
    {
        m_imageMask = vtkSmartPointer<vtkImageData>::New();
    }
    m_imageMask->DeepCopy(iim->GetOutput());
    emit signal_operationFinished();
}

void ImageProcessWidget::blur(vtkSmartPointer<vtkImageData> image, int blurX, int blurY)
{
    if (!image)
        return;
    int* dims = image->GetDimensions();
    cv::Mat cvMat(dims[1], dims[0], CV_8UC1,image->GetScalarPointer());

    cv::GaussianBlur(cvMat, cvMat, cv::Size(blurX, blurY), 0);
    for (int m = 0; m < dims[1]; ++m)
    {
        for (int n = 0; n < dims[0]; ++n)
        {
            uchar* pPixel = (uchar*)(image->GetScalarPointer(n, m, 0));
            *pPixel = cvMat.at<uchar>(cv::Point(n, m));
        }
    }

    image->Modified();
}

void ImageProcessWidget::threshold(vtkSmartPointer<vtkImageData> image, int param_threshold, int maxval)
{
    if (!image)
        return;
    int* dims = image->GetDimensions();
    cv::Mat cvMat(dims[1], dims[0], CV_8UC1, image->GetScalarPointer());
    cv::threshold(cvMat, cvMat, param_threshold,maxval,cv::THRESH_BINARY);
    for (int m = 0; m < dims[1]; ++m)
    {
        for (int n = 0; n < dims[0]; ++n)
        {
            uchar* pPixel = (uchar*)(image->GetScalarPointer(n, m, 0));
            *pPixel = cvMat.at<uchar>(cv::Point(n, m));
        }
    }
    image->Modified();
}

void ImageProcessWidget::canny(vtkSmartPointer<vtkImageData> image,int threshold1,int threshold2)
{
    if (!image)
        return;
    int* dims = image->GetDimensions();
    cv::Mat cvMat(dims[1], dims[0], CV_8UC1, image->GetScalarPointer());
    cv::Canny(cvMat, cvMat, threshold1, threshold2);
    for (int m = 0; m < dims[1]; ++m)
    {
        for (int n = 0; n < dims[0]; ++n)
        {
            uchar* pPixel = (uchar*)(image->GetScalarPointer(n, m, 0));
            *pPixel = cvMat.at<uchar>(cv::Point(n, m));
        }
    }
    
    image->Modified();
}

void ImageProcessWidget::connectedComponentsWithStats(vtkSmartPointer<vtkImageData> image)
{
    if (!image)
        return;
    int* dims = image->GetDimensions();
    cv::Mat cvMat(dims[1], dims[0], CV_8UC1, image->GetScalarPointer());
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
            uchar* pPixel = (uchar*)(image->GetScalarPointer(n, m, 0));
            *pPixel = cvMat.at<uchar>(cv::Point(n, m));
        }
    }
    image->Modified();
}

void ImageProcessWidget::floodFill(vtkSmartPointer<vtkImageData> image)
{
    if (!image)
        return;
    int* dims = image->GetDimensions();
    cv::Mat cvMat(dims[1], dims[0], CV_8UC1,image->GetScalarPointer());
    cv::floodFill(cvMat, cv::Point(0, 0), cv::Scalar(255));
    image->Modified();
}


#include <QColorDialog>
void ImageProcessWidget::on_pushButton_setColor_clicked(){
    QColor color=QColorDialog::getColor(Qt::green, this, "请选择面绘制的颜色");
    if (color.isValid()) {
        emit signal_switchRenderColor(color);
    }
}
#include <vtkDecimatePro.h>
void ImageProcessWidget::on_pushButton_saveMesh_clicked()
{
    if(!m_polyData)
    {
        QMessageBox::warning(this, "Warning", "current mesh is empty!");
        return;
    }
    QString curdate = QDateTime::currentDateTime().toString("yyyy_mm_dd_hh_mm_ss");
    QString savepath = QFileDialog::getSaveFileName(this, "Save Mesh", "D:/"+ curdate+".stl");
    if(savepath.isEmpty())
        return;
    QFileInfo saveInfo(savepath);
    if(saveInfo.isDir())
    {
        QMessageBox::warning(this, "Warning", "please input a filename!");
        return;
    }
    if(saveInfo.suffix()!="stl")
    {
        QMessageBox::warning(this, "Warning", "the suffix of file should be stl!");
        return;
    }
    auto decimate = vtkSmartPointer<vtkDecimatePro>::New();
    decimate->SetInputData(m_polyData);
    decimate->SetTargetReduction(.8);
    decimate->Update();
    vtkSmartPointer<vtkSTLWriter> writer = vtkSmartPointer<vtkSTLWriter>::New();
    writer->SetInputData(decimate->GetOutput());
    writer->SetFileName(saveInfo.absoluteFilePath().toStdString().c_str());
    int rt=writer->Write();
    if(rt==0)
    {
        QMessageBox::warning(this, "Warning", "failed to save file!");
        return;
    }
    QMessageBox::information(this, "Information", "success to save file!");
}

void ImageProcessWidget::on_pushButton_saveImage_clicked()
{
    if (!m_imageData)
        return;
    QString savePath = QFileDialog::getExistingDirectory(this, "Save", "D:/");
    if(savePath.isEmpty())
        return;
    vtkSmartPointer<vtkImageData> image = m_imageData;
    if(m_imageMask)
    {
        vtkSmartPointer<vtkImageMathematics> add = vtkSmartPointer<vtkImageMathematics>::New();
        add->SetInput1Data(m_imageMask);
        add->SetInput2Data(m_imageData);
        add->SetOperationToMultiply();
        add->Update();
        image = add->GetOutput();
    }
    itk::VTKImageToImageFilter<ImageType>::Pointer importer = itk::VTKImageToImageFilter<ImageType>::New();
    importer->SetInput(image);
    try
    {
        importer->Update();
    }
    catch (itk::ExceptionObject &e)
    {
        qDebug() << "error import image:" << e.what();
        return;
    }
    typedef itk::NumericSeriesFileNames NameGeneratorType;
    NameGeneratorType::Pointer nameGenerator = NameGeneratorType::New();
    std::string format = savePath.toStdString();
    format += "/Z%04d.tif";
    nameGenerator->SetSeriesFormat(format.c_str());
    ImageType::ConstPointer inputImage = importer->GetOutput();
    ImageType::RegionType region = inputImage->GetLargestPossibleRegion();
    ImageType::IndexType start = region.GetIndex();
    ImageType::SizeType size = region.GetSize();
    const unsigned int firstSlice = start[2];
    const unsigned int lastSlice = start[2] + size[2] - 1;
    nameGenerator->SetStartIndex(firstSlice);
    nameGenerator->SetEndIndex(lastSlice);
    nameGenerator->SetIncrementIndex(1);

    typedef itk::Image< PixelType, 2 > Image2DType;
    typedef itk::ImageSeriesWriter< ImageType, Image2DType > WriterType;
    WriterType::Pointer writer = WriterType::New();
    writer->SetInput(inputImage);
    writer->SetFileNames(nameGenerator->GetFileNames());
    writer->SetImageIO(itk::TIFFImageIO::New());
    try
    {
        writer->Update();
    }
    catch (itk::ExceptionObject& excp)
    {
        std::cerr << "Exception thrown while reading the image" << std::endl;
        std::cerr << excp << std::endl;
        QMessageBox::warning(this, "Warning", "failed to save file!");
        return;
    }
    QMessageBox::information(this, "Information", "success to save file!");
}

void ImageProcessWidget::on_pushButton_threshold_2d_clicked()
{
    int _threshold = ui->spinBox_threshold_2d->value();
    threshold(m_imageMask2d, _threshold, 255);
    m_imageactor->SetInputData(m_imageMask2d);
    m_renderwindow->Render();
}

void ImageProcessWidget::on_pushButton_canny_2d_clicked()
{
    canny(m_imageMask2d, 0, 255);
    m_imageactor->SetInputData(m_imageMask2d);
    m_renderwindow->Render();
}

void ImageProcessWidget::on_pushButton_blur_2d_clicked()
{
    blur(m_imageMask2d,11,11);
    m_imageactor->SetInputData(m_imageMask2d);
    m_renderwindow->Render();
}

void ImageProcessWidget::on_pushButton_extract_2d_clicked()
{
    connectedComponentsWithStats(m_imageMask2d);
    m_imageactor->SetInputData(m_imageMask2d);
    m_renderwindow->Render();
}

void ImageProcessWidget::on_pushButton_fill_2d_clicked()
{
    floodFill(m_imageMask2d);
    m_imageactor->SetInputData(m_imageMask2d);
    m_renderwindow->Render();
}

void ImageProcessWidget::on_pushButton_segment_2d_clicked()
{
    if (!m_imageData || !m_imageMask2d)
        return;
    int* dims = m_imageData->GetDimensions();
    cv::Mat cvMat(dims[1], dims[0], CV_8UC1, m_imageData->GetScalarPointer());
    cv::Mat cvMatMask(dims[1], dims[0], CV_8UC1, m_imageMask2d->GetScalarPointer());
    for (int i = 0; i < cvMatMask.rows; ++i) {
        for (int j = 0; j < cvMatMask.cols; ++j) {
            uchar grayValue = cvMatMask.at<uchar>(i, j);
            if (grayValue==0)
                cvMat.at<uchar>(i, j) = 0;
        }
    }
    m_imageData->Modified();
    m_imageactor->SetInputData(m_imageData);
    m_renderwindow->Render();
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
