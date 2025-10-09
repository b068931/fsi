#ifndef MAIN_WINDOW_MESSAGES_H
#define MAIN_WINDOW_MESSAGES_H

#include <QTTranslation>

constexpr const char* g_Context = "Windows::MainWindow";
constexpr const char* g_Messages[] = {
    //: Default message in working directory label.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Working directory not set."),

    //: Default message displayed in the status bar for the environment
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Not started"),

    //: Default message displayed in the status bar for the translator
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Not started"),

    //: Title for the "Open Working Directory" dialog.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Select Working Directory"),

    //: Title for the "Open File" dialog.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Select File"),

    //: File filter for source code files.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Source Files (*.tfsi *.fsi);;All Files (*.*)"),

    //: Status tip after choosing a language in the translator menu.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Changed application language. A few displayed strings may linger."),

    //: Status tip after user tries to close the application with unsaved files, and then cancels the action.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Close canceled. Unsaved changes remain.")
};

enum MessageKeys {
    g_StatusBarDefaultWorkingDirectory = 0,
    g_StatusBarDefaultEnvironmentState = 1,
    g_StatusBarDefaultTranslatorState = 2,
    g_DialogTitleOpenWorkingDirectory = 3,
    g_DialogTitleOpenFile = 4,
    g_DialogFilterSourceFiles = 5,
    g_StatusTipLanguageChanged = 6,
    g_StatusTipCloseCanceled = 7
};

#endif