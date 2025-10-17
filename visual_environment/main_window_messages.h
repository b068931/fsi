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
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Close canceled. Unsaved changes remain."),

    //: Status tip after user cancels the working directory selection dialog.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Working directory selection canceled."),

    //: Status tip after file save operation fails.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "File save canceled."),

    //: Status tip after file save operation succeeds.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "File saved successfully."),

    //: Status tip after file close operation is canceled.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "File close canceled."),

    //: Status tip after user wants to do an action with file when no file is selected.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "No file selected."),

    //: Status tip just after starting the application.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Ready to work."),

    //: The name of the translation result file.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "build.bfsi"),

    //: Status tip after translation flag option dialog was cancelled, this cancels program translation.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Action Cancelled"),

    //: Status tip after execution environment configuration file input dialog was cancelled, this cancels program execution.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Action Cancelled"),

    //: The name of the execution environment log file.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "runtime.log"),

    //: Status tip after FSI translator has just been started.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Running..."),

    //: Status tip after FSI translator finished successfully.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Success!"),

    //: Status tip after FSI translator finished with errors.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Errors Detected"),

    //: Status tip after FSI translator is already running, but user tries to start another one.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Already Running"),

    //: Title for the message box which pops up to catch user attention after they try to start a translator, while there is another running instance.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Instance is Already Active"),

    //: Message for the message box which pops up to catch user attention after they try to start a translator, while there is another running instance.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "There is an already active instance of the FSI Translator. Close the previous window before starting another one."),

    //: Status tip after FSI translator crashes.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Instance Crashed"),

    //: Title for the message box which pops up to catch user attention after the FSI translator crashes.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Instance Crashed"),

    //: Message for the message box which pops up to catch user attention after the FSI translator crashes.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "The FSI Translator has crashed. Please restart the application."),

    //: Status tip after FSI translator failed to start.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Start Failed"),

    //: Title for the message box which pops when a child returns an unknown result.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Unknown Result"),

    //: Message for the message box which pops when a child returns an unknown result.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "The operation returned an unknown result. Most likely, the developer of this application has failed to handle some edge case."),

    //: Status tip after FSI Mediator has just been started.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Running..."),

    //: Status tip after FSI Mediator finished successfully.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Success!"),

    //: Status tip after FSI Mediator finished with errors.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Errors Detected"),

    //: Status tip after FSI Mediator is already running, but user tries to start another one.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Already Running"),

    //: Title for the message box which pops up to catch user attention after they try to start a mediator, while there is another running instance.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Instance is Already Active"),

    //: Message for the message box which pops up to catch user attention after they try to start a mediator, while there is another running instance.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "There is an already active instance of the FSI Mediator. Close the previous window before starting another one."),

    //: Status tip after FSI Mediator crashes.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Instance Crashed"),

    //: Title for the message box which pops up to catch user attention after the FSI Mediator crashes.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Instance Crashed"),

    //: Message for the message box which pops up to catch user attention after the FSI Mediator crashes.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "The FSI Mediator has crashed. Please restart the application."),

    //: Status tip after FSI Mediator failed to start.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Start Failed"),

    //: Status tip after user cancels save operation before translation.
    QT_TRANSLATE_NOOP("Windows::MainWindow", "Running old version of the file."),
};

enum MessageKeys {
    g_StatusBarDefaultWorkingDirectory = 0,
    g_StatusBarDefaultEnvironmentState = 1,
    g_StatusBarDefaultTranslatorState = 2,
    g_DialogTitleOpenWorkingDirectory = 3,
    g_DialogTitleOpenFile = 4,
    g_DialogFilterSourceFiles = 5,
    g_StatusTipLanguageChanged = 6,
    g_StatusTipCloseCanceled = 7,
    g_StatusTipWorkingDirectorySelectionCanceled = 8,
    g_StatusTipFileSaveCanceled = 9,
    g_StatusTipFileSaveSucceeded = 10,
    g_StatusTipFileCloseCanceled = 11,
    g_StatusTipNoFileSelected = 12,
    g_StatusTipStartup = 13,
    g_TranslationResultFileName = 14,
    g_StatusTipProgramTranslationCancelled = 15,
    g_StatusTipProgramExecutionCancelled = 16,
    g_EELogFileName = 17,
    g_StatusTipTranslatorStarted = 18,
    g_StatusTipTranslatorSuccess = 19,
    g_StatusTipTranslatorProgramHasErrors = 20,
    g_StatusTipTranslatorAlreadyRunning = 21,
    g_DialogTitleTranslatorAlreadyRunning = 22,
    g_DialogMessageTranslatorAlreadyRunning = 23,
    g_StatusTipTranslatorCrashed = 24,
    g_DialogTitleTranslatorCrashed = 25,
    g_DialogMessageTranslatorCrashed = 26,
    g_StatusTipTranslatorFailedToStart = 27,
    g_DialogTitleUnknownResult = 28,
    g_DialogMessageUnknownResult = 29,
    g_StatusTipExecutionEnvironmentStarted = 30,
    g_StatusTipExecutionEnvironmentSuccess = 31,
    g_StatusTipExecutionEnvironmentProgramHasErrors = 32,
    g_StatusTipExecutionEnvironmentAlreadyRunning = 33,
    g_DialogTitleExecutionEnvironmentAlreadyRunning = 34,
    g_DialogMessageExecutionEnvironmentAlreadyRunning = 35,
    g_StatusTipExecutionEnvironmentCrashed = 36,
    g_DialogTitleExecutionEnvironmentCrashed = 37,
    g_DialogMessageExecutionEnvironmentCrashed = 38,
    g_StatusTipExecutionEnvironmentFailedToStart = 39,
    g_StatusTipTranslationWithoutSave = 40
};

#endif