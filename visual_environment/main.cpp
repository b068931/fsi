#ifndef NDEBUG
#include <vld.h>
#endif

#include <QtWidgets/QApplication>
#include <QFile>
#include <QTextStream>
#include "main_window.h"

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);

    // Load and apply stylesheet
    QFile styleFile(":/styles/dark_theme.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream styleStream(&styleFile);
        application.setStyleSheet(styleStream.readAll());
        styleFile.close();
    }

    Windows::MainWindow mainWindow;
    mainWindow.showMaximized();

    return application.exec();
}
