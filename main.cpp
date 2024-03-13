#include <QApplication>

#include "mainwindow.h"

#include <vtkAutoInit.h>
#include <vtkOutputWindow.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);

int main(int argc, char* argv[])
{
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication application(argc, argv);
	vtkOutputWindow::GlobalWarningDisplayOff();
	MainWindow w;
	w.show();
	return application.exec();
}
