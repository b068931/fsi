#ifndef FSI_TOOLS_MESSAGES_H
#define FSI_TOOLS_MESSAGES_H

#include <QTTranslation>

constexpr const char* g_Context = "Components::FSITools";
constexpr const char* g_Messages[] = {
    //: Title for the input dialog message box shown when user is asked to provide a flag for the translator debug options.
    QT_TRANSLATE_NOOP("Components::FSITools", "Choose a Debug Option"),
    
    //: Message for the input dialog message box shown when user is asked to provide a flag for the translator debug options.
    QT_TRANSLATE_NOOP("Components::FSITools", "Debug information flag for the translator:"),

    //: Title for the input dialog message box shown when user is asked to choose a configuration file for the execution environment.
    QT_TRANSLATE_NOOP("Components::FSITools", "Choose a Configuration File"),

    //: Message for the input dialog message box shown when user is asked to choose a configuration file for the execution environment.
    QT_TRANSLATE_NOOP("Components::FSITools", "Configuration file describes which modules will be loaded:"),

    //: Console window title for the translator child process.
    QT_TRANSLATE_NOOP("Components::FSITools", "FSI Translator"),

    //: Console window title for the execution environment child process.
    QT_TRANSLATE_NOOP("Components::FSITools", "FSI Mediator"),
};

enum MessageKeys {
    g_DebugOptionDialogTitle = 0,
    g_DebugOptionDialogMessage = 1,
    g_EEConfigurationFileDialogTitle = 2,
    g_EEConfigurationFileDialogMessage = 3,
    g_TranslatorConsoleWindowTitle = 4,
    g_EEConsoleWindowTitle = 5,
};

#endif