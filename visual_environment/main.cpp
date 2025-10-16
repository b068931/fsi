#ifndef NDEBUG
//#include <vld.h>
#endif

#include <QtWidgets/QApplication>
#include <QFile>
#include <QTextStream>
#include "main_window.h"

// TODO: Consider adding command line arguments for opening specific files at startup.
// TODO: Consider implementing some mechanism to use exceptions for error handling.
//       Qt has very poor support for exceptions, but it might be possible to implement something.
//       For example, you are not allowed to throw exceptions from slots, but you can catch them before they leave the slot.
//       It might be possible to implement some mechanism which will automatically catch exceptions and push them to event queue, and then process them (show message box, etc.) in the main event loop.

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
