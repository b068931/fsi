#include <QFileSystemModel>
#include <QFileInfo>
#include <QShowEvent>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QInputDialog>
#include <QDir>

#include "text_editor.h"
#include "text_editor_messages.h"

namespace CustomWidgets {
    void TextEditor::showEvent(QShowEvent* event) {
        if (!event->spontaneous()) {
            // Set the default splitter ratio when the widget is shown for the first time.
            this->setDefaultSplitterRatio();
        }
    }

    bool TextEditor::event(QEvent* event) {
        if (event->type() == QEvent::LanguageChange) {
            this->onRetranslateUI();
        }

        return QWidget::event(event);
    }

    void TextEditor::onWorkingDirectoryItemDoubleClicked(const QModelIndex& index) noexcept {
        Q_ASSERT(this->workingDirectory != nullptr && "The working directory has not been set up.");

        QFileSystemModel* fileModel = qobject_cast<QFileSystemModel*>(this->workingDirectory->model());
        if (!fileModel)
            return;

        QString filePath = fileModel->filePath(index);
        QFileInfo fileInfo(filePath);

        // TreeView already handles directories (expanding/collapsing)
        if (fileInfo.isFile()) {
            this->openNewFile(filePath);
        }
    }

    void TextEditor::onTabSwitchShortcut() {
        Q_ASSERT(this->tabSelectionHistory.size() == 2 && "Unexpected history size.");
        
        if (this->tabSelectionHistory[1] > -1 && this->tabSelectionHistory[1] < this->fileTabs->count()) {
            this->fileTabs->setCurrentIndex(this->tabSelectionHistory[1]);
        }
    }

    void TextEditor::onFileChangedOutside(const QString& path) noexcept {
        this->onFileChangedOutsideRecursive(path, 0);
    }

    void TextEditor::onFileChangedOutsideRecursive(const QString& path, int depth) noexcept {
        Q_ASSERT(this->fileWatcher && "The file watcher has not been set up.");
        constexpr int maximumFileExistenceRetries = 4;

        // Start tracking the file again if it was removed from the watcher for some reason.
        if (!this->fileWatcher->files().contains(path)) {
            this->fileWatcher->addPath(path);
        }

        QFileInfo fileInfo(path);
        for (int index = 0; index < this->openFiles.size(); ++index) {
            OpenedFile& openFile = this->openFiles[index];
            if (openFile.filePath == path) {
                // Silently disregard this message if the file is temporary.
                // This may indicate that some piece of logic is broken. That is,
                // it decided to mark a file as temporary, but forgot to remove it from the watcher.
                if (openFile.isTemporary) {
                    if (!this->fileWatcher->removePath(path)) {
                        qWarning() << "Failed to remove path from file watcher:" << path;
                    }

                    qWarning() << "Received file changed notification for a temporary file. This indicates a logic error. File path:" << path;
                    return;
                }

                QPlainTextEdit* fileEditor = this->getEditorAtIndex(index);
                if (!fileInfo.exists()) {
                    QMessageBox messageBox(this);
                    messageBox.setWindowTitle(tr(g_Messages[g_MessageBoxFileRemovedTitle]));
                    messageBox.setText(tr(g_Messages[g_MessageBoxFileRemovedMessage]).arg(path));
                    messageBox.setIcon(QMessageBox::Question);
                    
                    QPushButton* yesButton = messageBox.addButton(QMessageBox::Yes);
                    QPushButton* noButton = messageBox.addButton(QMessageBox::No);
                    
                    QPushButton* tryAgainButton = nullptr;
                    if (depth < maximumFileExistenceRetries) {
                        tryAgainButton = messageBox.addButton(
                            tr(g_Messages[g_CheckAgainFileRemovedButton]),
                            QMessageBox::ActionRole
                        );

                        messageBox.setDefaultButton(tryAgainButton);
                    }
                    else {
                        messageBox.setDefaultButton(noButton);
                    }
                    
                    messageBox.exec();
                    QAbstractButton* clickedButton = messageBox.clickedButton();

                    if (clickedButton == tryAgainButton) {
                        this->onFileChangedOutsideRecursive(path, depth + 1);
                        return;
                    }
                    else if (clickedButton == yesButton) {
                        // Remove the tab and stop tracking the file.
                        this->fileTabs->removeTab(index);
                        this->openFiles.removeAt(index);
                    }
                    else {
                        fileEditor->document()->setModified(true);
                        openFile.isTemporary = true;
                    }
                }
                else {
                    QFile newFile(path);
                    if (!newFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                        QMessageBox::warning(
                        this,
                            tr(g_Messages[g_MessageBoxFileChangedOutsideErrorTitle]),
                            tr(g_Messages[g_MessageBoxFileChangedOutsideErrorMessage]).arg(path)
                        );

                        fileEditor->document()->setModified(true);
                        openFile.isTemporary = true;
                        if (!this->fileWatcher->removePath(path)) {
                            qWarning() << "Failed to remove path from file watcher:" << path;
                        }
                    }
                    else {
                        if (!fileEditor->document()->isModified() && !openFile.isTemporary) {
                            // This will get executed even when we modify and save the file ourselves.
                            QByteArray newBuffer = newFile.readAll();
                            if (fileEditor->document()->toPlainText().toUtf8() != newBuffer) {
                                fileEditor->setPlainText(newBuffer);
                            }

                            fileEditor->document()->setModified(false);
                            if (newFile.error() != QFile::NoError) {
                                QMessageBox::warning(
                                    this,
                                    tr(g_Messages[g_MessageBoxFileReadErrorTitle]),
                                    tr(g_Messages[g_MessageBoxFileReadErrorMessage]).arg(path)
                                );
                            }
                        }
                        else {
                            QMessageBox::StandardButton result = QMessageBox::question(
                                this,
                                tr(g_Messages[g_MessageBoxFileChangedOutsideTitle]),
                                tr(g_Messages[g_MessageBoxFileChangedOutsideMessage]).arg(path),
                                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                QMessageBox::No
                            );

                            if (result == QMessageBox::Yes) {
                                fileEditor->setPlainText(newFile.readAll());
                                fileEditor->document()->setModified(false);

                                if (newFile.error() != QFile::NoError) {
                                    QMessageBox::warning(
                                        this,
                                        tr(g_Messages[g_MessageBoxFileReadErrorTitle]),
                                        tr(g_Messages[g_MessageBoxFileReadErrorMessage]).arg(path)
                                    );
                                }
                            }
                            else {
                                fileEditor->document()->setModified(true);
                            }
                        }
                    }
                }

                return;
            }
        }

        // Stop tracking the file if we can't find it in the open files list.
        if (!this->fileWatcher->removePath(path)) {
            qWarning() << "Failed to remove path from file watcher:" << path;
        }
    }

    void TextEditor::onTabCloseRequested(int index) noexcept {
        this->closeFileAtIndex(index);
    }

    void TextEditor::onTabMoved(int from, int to) noexcept {
        Q_ASSERT(from > -1 && from < this->openFiles.size() && "The 'from' index is out of range.");
        Q_ASSERT(to > -1 && to < this->openFiles.size() && "The 'to' index is out of range.");

        this->openFiles.move(from, to);
    }

    void TextEditor::onTabChanged(int index) noexcept {
        Q_ASSERT(this->tabSelectionHistory.size() == 2 && "Unexpected history size.");

        this->tabSelectionHistory[1] = this->tabSelectionHistory[0];
        this->tabSelectionHistory[0] = index;
    }

    void TextEditor::onRetranslateUI() noexcept {
        Q_ASSERT(this->workingDirectory && "The working directory has not been set up.");
        Q_ASSERT(this->openAction && "The open action has not been set up.");
        Q_ASSERT(this->newFileAction && "The new file action has not been set up.");
        Q_ASSERT(this->newDirectoryAction && "The new directory action has not been set up.");
        Q_ASSERT(this->removeAction && "The remove action has not been set up.");

        this->workingDirectory->setStatusTip(
            tr(g_Messages[g_TooltipWorkingDirectoryView])
        );

        this->openAction->setText(tr(g_Messages[g_ContextMenuOpen]));
        this->newFileAction->setText(tr(g_Messages[g_ContextMenuNewFile]));
        this->newDirectoryAction->setText(tr(g_Messages[g_ContextMenuNewDirectory]));
        this->removeAction->setText(tr(g_Messages[g_ContextMenuRemove]));
    }

    void TextEditor::onWorkingDirectoryContextMenu(const QPoint& position) noexcept {
        Q_ASSERT(this->workingDirectory != nullptr && "The working directory has not been set up.");
        Q_ASSERT(this->workingDirectoryContextMenu && "The working directory context menu has not been set up.");
        Q_ASSERT(this->openAction && "The open action has not been set up.");
        Q_ASSERT(this->newFileAction && "The new file action has not been set up.");
        Q_ASSERT(this->newDirectoryAction && "The new directory action has not been set up.");
        Q_ASSERT(this->removeAction && "The remove action has not been set up.");

        QFileSystemModel* fileModel = qobject_cast<QFileSystemModel*>(this->workingDirectory->model());
        if (!fileModel) {
            return;
        }

        QModelIndex index = this->workingDirectory->indexAt(position);
        QString clickedPath;
        bool isDirectory = false;
        bool isValidItem = false;

        if (index.isValid()) {
            clickedPath = fileModel->filePath(index);

            QFileInfo fileInfo(clickedPath);
            isDirectory = fileInfo.isDir();
            isValidItem = true;
        }

        // Enable/disable actions based on what was clicked
        this->openAction->setEnabled(isValidItem);
        this->removeAction->setEnabled(isValidItem);

        // Show menu and get selected action
        QAction* selectedAction = this->workingDirectoryContextMenu->exec(
            this->workingDirectory->viewport()->mapToGlobal(position)
        );

        if (!selectedAction) {
            return;
        }

        // Handle selected action
        if (selectedAction == this->openAction) {
            if (isDirectory) {
                if (this->workingDirectory->isExpanded(index)) {
                    this->workingDirectory->collapse(index);
                }
                else {
                    this->workingDirectory->expand(index);
                }
            }
            else {
                this->openNewFile(clickedPath);
            }
        }
        else if (selectedAction == this->newFileAction) {
            QString parentDirectory;
            if (isValidItem && isDirectory) {
                parentDirectory = clickedPath;
            }
            else if (isValidItem && !isDirectory) {
                QFileInfo fileInfo(clickedPath);
                parentDirectory = fileInfo.absolutePath();
            }
            else {
                parentDirectory = this->getWorkingDirectoryPath();
            }

            bool ok;
            QString fileName = QInputDialog::getText(
                this,
                tr(g_Messages[g_InputDialogNewFileTitle]),
                tr(g_Messages[g_InputDialogNewFileLabel]),
                QLineEdit::Normal,
                QString(),
                &ok
            );

            if (ok && !fileName.isEmpty()) {
                QString newFilePath = parentDirectory + QDir::separator() + fileName;
                QFile newFile(newFilePath);
                
                if (newFile.open(QIODevice::NewOnly | QIODevice::Text)) {
                    newFile.close();
                    this->openNewFile(newFilePath);
                }
                else {
                    QMessageBox::warning(
                        this,
                        tr(g_Messages[g_ErrorFileSystemTitle]),
                        tr(g_Messages[g_ErrorFileCreationMessage]).arg(newFilePath)
                    );
                }
            }
        }
        else if (selectedAction == this->newDirectoryAction) {
            // Determine parent directory
            QString parentDirectory;
            if (isValidItem && isDirectory) {
                parentDirectory = clickedPath;
            }
            else if (isValidItem && !isDirectory) {
                QFileInfo fileInfo(clickedPath);
                parentDirectory = fileInfo.absolutePath();
            }
            else {
                parentDirectory = this->getWorkingDirectoryPath();
            }

            bool ok;
            QString directoryName = QInputDialog::getText(
                this,
                tr(g_Messages[g_InputDialogNewDirectoryTitle]),
                tr(g_Messages[g_InputDialogNewDirectoryLabel]),
                QLineEdit::Normal,
                QString(),
                &ok
            );

            if (ok && !directoryName.isEmpty()) {
                QString newDirectoryPath = parentDirectory + QDir::separator() + directoryName;
                QDir directory;
                
                if (!directory.mkpath(newDirectoryPath)) {
                    QMessageBox::warning(
                        this,
                        tr(g_Messages[g_ErrorFileSystemTitle]),
                        tr(g_Messages[g_ErrorDirectoryCreationMessage]).arg(newDirectoryPath)
                    );
                }
            }
        }
        else if (selectedAction == this->removeAction) {
            QString confirmMessage;
            if (isDirectory) {
                confirmMessage = tr(g_Messages[g_ConfirmRemovalDirectoryMessage]).arg(clickedPath);
            }
            else {
                confirmMessage = tr(g_Messages[g_ConfirmRemovalFileMessage]).arg(clickedPath);
            }

            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                tr(g_Messages[g_ConfirmRemovalTitle]),
                confirmMessage,
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                QMessageBox::No
            );

            if (reply == QMessageBox::Yes) {
                bool success = false;
                
                if (isDirectory) {
                    QDir dir(clickedPath);
                    success = dir.removeRecursively();
                }
                else {
                    QFile file(clickedPath);
                    success = file.remove();
                }

                if (!success) {
                    QMessageBox::warning(
                        this,
                        tr(g_Messages[g_ErrorFileSystemTitle]),
                        tr(g_Messages[g_ErrorRemovalMessage]).arg(clickedPath)
                    );
                }
            }
        }
    }
}
