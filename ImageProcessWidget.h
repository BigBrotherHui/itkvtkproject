#ifndef IMAGEPROCESSWIDGET_H
#define IMAGEPROCESSWIDGET_H

#include <QDockWidget>

namespace Ui {
class ImageProcessWidget;
}

class ImageProcessWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit ImageProcessWidget(QWidget *parent = nullptr);
    ~ImageProcessWidget();
    void closeEvent(QCloseEvent* event) override;
private:
    Ui::ImageProcessWidget *ui;
};

#endif // IMAGEPROCESSWIDGET_H
