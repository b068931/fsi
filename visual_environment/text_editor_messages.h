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

    //: Title for the message box which pops up when a file has been modified outside the text editor.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "File Changed"),

    //: Text for the message box which pops up when a file has been modified outside the text editor.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "File \"%1\" has been modified outside the editor. Reload?"),

    //: Title for the message box which pops up when a file has been modified outside the text editor. But the file can't be opened.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "File Changed Error"),

    //: Text for the message box which pops up when a file has been modified outside the text editor. But the file can't be opened.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "File \"%1\" has been modified outside the editor. However, it can't be reloaded."),

    //: Title for the message box which pops up when a file has been removed outside the text editor.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "File Removed"),

    //: Text for the message box which pops up when a file has been removed outside the text editor.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "File \"%1\" has been removed outside the editor. Remove it?"),

    //: Title for the message box which pops up when a file can't be opened for writing during save operation.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "File Save Error"),

    //: Text for the message box which pops up when a file can't be opened for writing during save operation.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "Couldn't open file \"%1\" for writing. You might want to save the file in a different place."),

    //: Save file dialog title.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "Save File"),

    //: Save file dialog file type filter options.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "FSI Source File (*.tfsi *.fsi);;All Files (*)"),
};

enum MessageKeys {
    g_TooltipWorkingDirectoryView = 0,
    g_MessageBoxFileOpenErrorTitle = 1,
    g_MessageBoxFileOpenErrorMessage = 2,
    g_MessageBoxFileChangedOutsideTitle = 3,
    g_MessageBoxFileChangedOutsideMessage = 4,
    g_MessageBoxFileChangedOutsideErrorTitle = 5,
    g_MessageBoxFileChangedOutsideErrorMessage = 6,
    g_MessageBoxFileRemovedTitle = 7,
    g_MessageBoxFileRemovedMessage = 8,
    g_MessageBoxFileSaveErrorTitle = 9,
    g_MessageBoxFileSaveErrorMessage = 10,
    g_SaveFileDialogTitle = 11,
    g_SaveFileDialogFilter = 12
};

#endif
