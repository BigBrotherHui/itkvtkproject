#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <itkImageSeriesReader.h>
#include <itkNumericSeriesFileNames.h>
#include <itkImageToVTKImageFilter.h>

#include <itkCastImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>
#include <itkTIFFImageIO.h>
#include <itkCommand.h>
#include <QProgressDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include "ImageProcessWidget.h"
#include <vtkPolyDataMapper.h>
#include <QDebug>
#include <vtkProperty.h>
class CommandProgressUpdate : public itk::Command
{
public:
    typedef  CommandProgressUpdate   Self;
    typedef  itk::Command            Superclass;
    typedef  itk::SmartPointer<Self> Pointer;
    itkNewMacro(Self);
protected:
    CommandProgressUpdate()
    {
    };
public:
    void Execute(itk::Object* caller, const itk::EventObject& event)
    {
        Execute((const itk::Object*)caller, event);
    }
    void Execute(const itk::Object* object, const itk::EventObject& event)
    {
        const itk::ProcessObject* filter = dynamic_cast<const itk::ProcessObject*>(object);
        if (!itk::ProgressEvent().CheckEvent(&event)) {
            return;
        }
        this->dialog->setValue(static_cast<int>(filter->GetProgress() * 100));
    }
    void setProgressDialog(QProgressDialog* dialog)
    {
        this->dialog = dialog;
    }
private:
    QProgressDialog* dialog;
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_cbk = vtkSmartPointer<VTKResliceCursorCallback>::New();
    m_cbk->setMPRXWidget(ui->widget_axial);
    m_cbk->setMPRYWidget(ui->widget_coronal);
    m_cbk->setMPRZWidget(ui->widget_sagittal);

    ui->widget_axial->setImageOrient(0);
    ui->widget_coronal->setImageOrient(1);
    ui->widget_sagittal->setImageOrient(2);

    ui->widget_coronal->setResliceCursor(ui->widget_axial->getResliceCursor());
    ui->widget_sagittal->setResliceCursor(ui->widget_axial->getResliceCursor());

    ui->widget_axial->installEventFilter(this);
    ui->widget_coronal->installEventFilter(this);
    ui->widget_sagittal->installEventFilter(this);
    ui->widget_3d->installEventFilter(this);

    std::vector<QWidget*> except;
    except.push_back(ui->pushButton_openFile);
    except.push_back(ui->pushButton_openDir);
    setButtonsEnable(0, except);

    take_over();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::RenderAllVtkRenderWindows()
{
    ui->widget_axial->renderWindow()->Render();
    ui->widget_coronal->renderWindow()->Render();
    ui->widget_sagittal->renderWindow()->Render();
    ui->widget_3d->renderWindow()->Render();
}

void MainWindow::on_pushButton_openFile_clicked()
{
    QString path = QFileDialog::getOpenFileName(this, "open file", "D:/");
    if (path.isEmpty())
        return;
    QProgressDialog* progressDialog = createProgressDialog(tr("Loading"), tr("Loading Image, Please Wait..."), 101);
    progressDialog->setWindowFlag(Qt::WindowStaysOnTopHint);
    bool ret = loadImagesFromSingleFile(path, progressDialog);
    progressDialog->deleteLater();
    if (!ret)
    {
        QMessageBox::warning(this, "Warning", "failed to import image!");
        return;
    }
    //ui->widget_axial->setImageData(m_imagedata);
    //ui->widget_coronal->setImageData(m_imagedata);
    //ui->widget_sagittal->setImageData(m_imagedata);
    //ui->widget_3d->setImageData(m_imagedata);

    ui->widget_3d->switchCameraView(ViewPort3D::VIEWPORT3D_FRONT);
    setButtonsEnable(1,ui->pushButton_openDir);
    m_isSingleFile = 1;
    on_pushButton_imageProcess_clicked();
}

void MainWindow::on_pushButton_openDir_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this, "open directory", "D:/");
    if (path.isEmpty())
        return;
    QProgressDialog* progressDialog = createProgressDialog(tr("Loading"), tr("Loading Images, Please Wait..."), 101);
    progressDialog->setWindowFlag(Qt::WindowStaysOnTopHint);
    bool ret = loadImagesFromDirectory(path, progressDialog);
    progressDialog->deleteLater();
    if (!ret)
    {
        QMessageBox::warning(this, "Warning", "failed to import images!");
        return;
    }
    ui->widget_axial->setImageData(m_imagedata);
    ui->widget_coronal->setImageData(m_imagedata);
    ui->widget_sagittal->setImageData(m_imagedata);
    ui->widget_3d->setImageData(m_imagedata);

    ui->widget_3d->switchCameraView(ViewPort3D::VIEWPORT3D_FRONT);

    setButtonsEnable(1,ui->pushButton_openFile);
}

void MainWindow::on_comboBox_blendMode_currentIndexChanged(int index)
{
    ui->widget_3d->setBlendMode(index);
    ui->widget_3d->renderWindow()->Render();
}

void MainWindow::on_pushButton_reset_clicked()
{
    ui->widget_3d->switchCameraView(ViewPort3D::VIEWPORT3D_FRONT);
    ui->widget_axial->resetResliceCursor();
    ui->widget_coronal->resetResliceCursor();
	ui->widget_sagittal->resetResliceCursor();
}

void MainWindow::slot_setRenderColor(QColor color) {
    static_cast<vtkActor*>(ui->widget_3d->getActor("marchingCubesActor"))->GetProperty()->SetColor(color.red() / 255.,
        color.green() / 255., color.blue() / 255.);
    RenderAllVtkRenderWindows();
}

void MainWindow::on_pushButton_imageProcess_clicked()
{
    if (!m_imageProcessWidget) {
        m_imageProcessWidget = new ImageProcessWidget(this);
        connect(m_imageProcessWidget, &ImageProcessWidget::signal_operationFinished, this, &MainWindow::slot_operationFinished);
        connect(m_imageProcessWidget, &ImageProcessWidget::signal_volumeVisible, this, &MainWindow::slot_volumeVisible);
        connect(m_imageProcessWidget, &ImageProcessWidget::signal_switchRenderColor, this, &MainWindow::slot_setRenderColor);
    }
    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea,m_imageProcessWidget);
    if(m_isSingleFile)
    {
        m_imageProcessWidget->setCurrentPageTo2DImageProcess();
        m_imageProcessWidget->setImageData(m_imagedata2d);
    }
    else
    {
        m_imageProcessWidget->setCurrentPageTo3DImageProcess();
        m_imageProcessWidget->setImageData(m_imagedata);
    }
    m_imageProcessWidget->show();
}

void MainWindow::slot_operationFinished()
{
    if (m_imageProcessWidget->GetPolyDataResult())
    {
        if(!ui->widget_3d->getActor("marchingCubesActor"))
        {
            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            actor->SetMapper(mapper);
            actor->GetProperty()->SetColor(0, 1, 0);
            ui->widget_3d->addActor("marchingCubesActor", actor);
        }
        static_cast<vtkPolyDataMapper*>(static_cast<vtkActor*>(ui->widget_3d->getActor("marchingCubesActor"))->GetMapper())
    	->SetInputData(m_imageProcessWidget->GetPolyDataResult());
    }
    if(m_imageProcessWidget->GetImageMask())
    {
        ui->widget_3d->setImageMask(m_imageProcessWidget->GetImageMask());
    }
    RenderAllVtkRenderWindows();
}

void MainWindow::slot_volumeVisible(int v)
{
    ui->widget_3d->getActor("volume")->SetVisibility(v);
    ui->widget_3d->renderWindow()->Render();
}

bool MainWindow::loadImagesFromDirectory(QString path, QProgressDialog* dialog)
{
    constexpr unsigned int Dimension = 3;
    using PixelType = unsigned short;
    using ImageType = itk::Image<PixelType, Dimension>;
    using ReaderType = itk::ImageSeriesReader<ImageType>;
    using ImageIOType = itk::TIFFImageIO;

    ImageIOType::Pointer tiffIO = ImageIOType::New();
    typedef itk::NumericSeriesFileNames NameGeneratorType;
    auto fileinfolist=QDir(path).entryInfoList();
    std::vector<std::string> filenames;
    static int i = 0;
    for(auto file: fileinfolist)
    {
        if (i++ == 300)
            break;
        if (file.suffix() == "tif")
        {
            filenames.push_back(file.absoluteFilePath().toStdString());
        }
    }
    /*NameGeneratorType::Pointer nameGenerator = NameGeneratorType::New();
    nameGenerator->SetSeriesFormat(path.toStdString() + "/Z%04d.tif");
    nameGenerator->SetStartIndex(49);
    nameGenerator->SetEndIndex(349);
    nameGenerator->SetIncrementIndex(1);
    std::vector<std::string> filenames = nameGenerator->GetFileNames();*/
    std::size_t numberOfFileNames = filenames.size();

    ReaderType::Pointer reader = ReaderType::New();
    reader->SetImageIO(tiffIO);
    reader->SetFileNames(filenames);
    CommandProgressUpdate::Pointer observer = CommandProgressUpdate::New();
    observer->setProgressDialog(dialog);
    reader->AddObserver(itk::ProgressEvent(), observer);
    try
    {
        reader->Update();
    }
    catch (const itk::ExceptionObject& e)
    {
        std::cerr << "exception in file reader " << std::endl;
        std::cerr << e << std::endl;
        return false;
    }
    using OutputImageType = itk::Image<unsigned char, 3>;
    using RescaleType = itk::RescaleIntensityImageFilter<ImageType, ImageType>;
    auto rescale = RescaleType::New();
    rescale->SetInput(reader->GetOutput());
    rescale->SetOutputMinimum(0);
    rescale->SetOutputMaximum(itk::NumericTraits<unsigned char>::max());
    rescale->Update();

    using CastImageFilterType = itk::CastImageFilter<ImageType, OutputImageType>;
    auto filter = CastImageFilterType::New();
    filter->SetInput(rescale->GetOutput());
    filter->Update();
    using ImageToVTKImageFilterType = itk::ImageToVTKImageFilter<OutputImageType>;
    auto imageToVTKImageFilter = ImageToVTKImageFilterType::New();
    imageToVTKImageFilter->SetInput(filter->GetOutput());
    try {
        imageToVTKImageFilter->Update();
    }
    catch (const itk::ExceptionObject& error) {
        std::cerr << "Error: " << error << std::endl;
        return false;
    }
    if (!m_imagedata)
    {
        m_imagedata = vtkSmartPointer<vtkImageData>::New();
    }
    m_imagedata->DeepCopy(imageToVTKImageFilter->GetOutput());
    return true;
}

bool MainWindow::loadImagesFromSingleFile(QString path, QProgressDialog* dialog)
{
    constexpr unsigned int Dimension = 2;
    using PixelType = unsigned short;
    using ImageType = itk::Image<PixelType, Dimension>;
    using ReaderType = itk::ImageFileReader<ImageType>;
    using ImageIOType = itk::TIFFImageIO;

    ImageIOType::Pointer tiffIO = ImageIOType::New();
    ReaderType::Pointer reader = ReaderType::New();
    reader->SetImageIO(tiffIO);
    reader->SetFileName(path.toStdString().c_str());
    CommandProgressUpdate::Pointer observer = CommandProgressUpdate::New();
    observer->setProgressDialog(dialog);
    reader->AddObserver(itk::ProgressEvent(), observer);
    try
    {
        reader->Update();
    }
    catch (const itk::ExceptionObject& e)
    {
        std::cerr << "exception in file reader " << std::endl;
        std::cerr << e << std::endl;
        return false;
    }
    using OutputImageType = itk::Image<unsigned char, 2>;
    using RescaleType = itk::RescaleIntensityImageFilter<ImageType, ImageType>;
    auto rescale = RescaleType::New();
    rescale->SetInput(reader->GetOutput());
    rescale->SetOutputMinimum(0);
    rescale->SetOutputMaximum(itk::NumericTraits<unsigned char>::max());
    rescale->Update();

    using CastImageFilterType = itk::CastImageFilter<ImageType, OutputImageType>;
    auto filter = CastImageFilterType::New();
    filter->SetInput(rescale->GetOutput());
    filter->Update();
    using ImageToVTKImageFilterType = itk::ImageToVTKImageFilter<OutputImageType>;
    auto imageToVTKImageFilter = ImageToVTKImageFilterType::New();
    imageToVTKImageFilter->SetInput(filter->GetOutput());
    try {
        imageToVTKImageFilter->Update();
    }
    catch (const itk::ExceptionObject& error) {
        std::cerr << "Error: " << error << std::endl;
        return false;
    }
    if (!m_imagedata2d)
    {
        m_imagedata2d = vtkSmartPointer<vtkImageData>::New();
    }
    m_imagedata2d->DeepCopy(imageToVTKImageFilter->GetOutput());
    return true;
}

QProgressDialog* MainWindow::createProgressDialog(QString title, QString prompt, int range)
{
    QProgressDialog* progressDialog = new QProgressDialog;
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(100);
    progressDialog->setWindowTitle(title);
    progressDialog->setLabelText(prompt);
    progressDialog->setCancelButtonText("Cancel");
    progressDialog->setRange(0, range);
    progressDialog->setCancelButton(nullptr);
    return progressDialog;
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent* ev = static_cast<QMouseEvent*>(event);
        if (ev->button() != Qt::LeftButton)
            return false;
        changeLayout(dynamic_cast<QWidget*>(watched));
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::changeLayout(QWidget* w)
{
    if (!w)
        return;
    QVector<QWidget*> widgetsContainer;
    widgetsContainer.push_back(ui->widget_axial);
    widgetsContainer.push_back(ui->widget_coronal);
    widgetsContainer.push_back(ui->widget_sagittal);
    widgetsContainer.push_back(ui->widget_3d);
    if (widgetsContainer.indexOf(w) == -1)
		return;
    if(m_ismax)
    {
        for (auto ww : widgetsContainer)
        {
            if (ww->isHidden())
                ww->show();
        }

    }else
    {
        for (auto ww : widgetsContainer)
        {
            if (ww!=w && ww->isVisible())
                ww->hide();
        }
    }
    m_ismax = !m_ismax;
}

void MainWindow::take_over()
{

}

void MainWindow::setButtonsEnable(bool f, QWidget* except)
{
    ui->pushButton_imageProcess->setEnabled(f);
    ui->pushButton_openFile->setEnabled(f);
    ui->pushButton_reset->setEnabled(f);
    ui->comboBox_blendMode->setEnabled(f);
    ui->pushButton_openDir->setEnabled(f);
    if (except)
        except->setEnabled(!f);
}

void MainWindow::setButtonsEnable(bool f, std::vector<QWidget*>except)
{
    ui->pushButton_imageProcess->setEnabled(f);
    ui->pushButton_openFile->setEnabled(f);
    ui->pushButton_reset->setEnabled(f);
    ui->comboBox_blendMode->setEnabled(f);
    ui->pushButton_openDir->setEnabled(f);
    if(except.size()!=0)
    {
	    for(int i=0;i<except.size();++i)
	    {
            except[i]->setEnabled(!f);
	    }
    }
}
