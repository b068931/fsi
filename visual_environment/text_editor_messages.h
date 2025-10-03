#ifndef TEXT_EDITOR_MESSAGES_H
#define TEXT_EDITOR_MESSAGES_H

#include <QTTranslation>

constexpr const char* g_Context = "CustomWidgets::TextEditor";
constexpr const char* g_Messages[] = {
    //: Tooltip message for the working directory view.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "Working directory view. Use double-click or context menu."),

    //: Title for the message box which pops up when text editor can't open a file.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "Error"),

    //: Text for the message box which pops up when text editor can't open a file.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "Couldn't open file \"%1\" for reading."),
};

enum MessageKeys {
    g_TooltipWorkingDirectoryView = 0,
    g_MessageBoxFileOpenErrorTitle = 1,
    g_MessageBoxFileOpenErrorMessage = 2
};

#endif
