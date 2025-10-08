#ifndef TEXT_EDITOR_MESSAGES_H
#define TEXT_EDITOR_MESSAGES_H

#include <QTTranslation>

constexpr const char* g_Context = "CustomWidgets::TextEditor";
constexpr const char* g_Messages[] = {
    //: Tooltip message for the working directory view.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "Working directory view. Use double-click or context menu."),

    //: Title for the message box which pops up when text editor can't open a file.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "Error: Opening New File"),

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

    //: Title for the message box which pops up when the editor can't write the whole file content to the disk.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "File Write Error"),

    //: Text for the message box which pops up when the editor can't write the whole file content to the disk.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "Couldn't write the whole file \"%1\" to disk. Try to save it to a different location."),

    //: Title for the message box which pops up when text editor can open a file, but can't read its content.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "File Read Error"),

    //: Text for the message box which pops up when text editor can open a file, but can't read its content.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "Couldn't read file \"%1\". You may see its contents partially or none at all."),

    //: Title for the message box which pops up when the user tries to close a file tab for the file which has not been saved.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "Unsaved Changes"),

    //: Text for the message box which pops up when the user tries to close a file tab for the file which has not been saved.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "You have unsaved changes in \"%1\". Do you want to save them before closing?"),

    //: A name for the temporary files created in the text editor.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "*Untitled*"),

    //: Text for the additional button in "File Removed" dialog. Asks the user whether they want the app to check if file exists again.
    QT_TRANSLATE_NOOP("CustomWidgets::TextEditor", "Check Again"),
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
    g_SaveFileDialogFilter = 12,
    g_MessageBoxFileWriteErrorTitle = 13,
    g_MessageBoxFileWriteErrorMessage = 14,
    g_MessageBoxFileReadErrorTitle = 15,
    g_MessageBoxFileReadErrorMessage = 16,
    g_MessageBoxFileCloseConfirmationTitle = 17,
    g_MessageBoxFileCloseConfirmationMessage = 18,
    g_TemporaryFileName = 19,
    g_CheckAgainFileRemovedButton = 20,
};

#endif
