#ifndef NDEBUG
#include <vld.h>
#endif

#include <QtWidgets/QApplication>
#include "main_window.h"

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);

    Windows::MainWindow mainWindow;
    mainWindow.showMaximized();

    return application.exec();
}
