#include "ImageProcessWidget.h"
#include "ui_ImageProcessWidget.h"

ImageProcessWidget::ImageProcessWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::ImageProcessWidget)
{
    ui->setupUi(this);
}

ImageProcessWidget::~ImageProcessWidget()
{
    delete ui;
}

void ImageProcessWidget::closeEvent(QCloseEvent* event)
{
    hide();
}
