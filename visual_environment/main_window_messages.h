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
};

enum MessageKeys {
    g_StatusBarDefaultWorkingDirectory = 0,
    g_StatusBarDefaultEnvironmentState = 1,
    g_StatusBarDefaultTranslatorState = 2
};

#endif