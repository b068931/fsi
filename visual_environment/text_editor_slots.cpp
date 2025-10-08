#include <QFileSystemModel>
#include <QFileInfo>
#include <QShowEvent>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>

#include "text_editor.h"
#include "text_editor_messages.h"

namespace CustomWidgets {
    void TextEditor::showEvent(QShowEvent* event) {
        if (!event->spontaneous()) {
            // Set the default splitter ratio when the widget is shown for the first time.
            this->setDefaultSplitterRatio();
        }
    }

    void TextEditor::onWorkingDirectoryItemDoubleClicked(const QModelIndex& index) {
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

    void TextEditor::onFileChangedOutside(const QString& path) {
        this->onFileChangedOutsideRecursive(path, 0);
    }

    void TextEditor::onFileChangedOutsideRecursive(const QString& path, int depth) {
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
                    QMessageBox msgBox(this);
                    msgBox.setWindowTitle(tr(g_Messages[MessageKeys::g_MessageBoxFileRemovedTitle]));
                    msgBox.setText(tr(g_Messages[MessageKeys::g_MessageBoxFileRemovedMessage]).arg(path));
                    msgBox.setIcon(QMessageBox::Question);
                    
                    QPushButton* yesButton = msgBox.addButton(QMessageBox::Yes);
                    QPushButton* noButton = msgBox.addButton(QMessageBox::No);
                    
                    QPushButton* tryAgainButton = nullptr;
                    if (depth < maximumFileExistenceRetries) {
                        tryAgainButton = msgBox.addButton(
                            tr(g_Messages[MessageKeys::g_CheckAgainFileRemovedButton]),
                            QMessageBox::ActionRole
                        );

                        msgBox.setDefaultButton(tryAgainButton);
                    }
                    else {
                        msgBox.setDefaultButton(noButton);
                    }
                    
                    msgBox.exec();
                    QAbstractButton* clickedButton = msgBox.clickedButton();

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
                            tr(g_Messages[MessageKeys::g_MessageBoxFileChangedOutsideErrorTitle]),
                            tr(g_Messages[MessageKeys::g_MessageBoxFileChangedOutsideErrorMessage]).arg(path)
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
                            fileEditor->setPlainText(newFile.readAll());
                            fileEditor->document()->setModified(false);

                            if (newFile.error() != QFile::NoError) {
                                QMessageBox::warning(
                                    this,
                                    tr(g_Messages[MessageKeys::g_MessageBoxFileReadErrorTitle]),
                                    tr(g_Messages[MessageKeys::g_MessageBoxFileReadErrorMessage]).arg(path)
                                );
                            }
                        }
                        else {
                            QMessageBox::StandardButton result = QMessageBox::question(
                                this,
                                tr(g_Messages[MessageKeys::g_MessageBoxFileChangedOutsideTitle]),
                                tr(g_Messages[MessageKeys::g_MessageBoxFileChangedOutsideMessage]).arg(path),
                                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                QMessageBox::No
                            );

                            if (result == QMessageBox::Yes) {
                                fileEditor->setPlainText(newFile.readAll());
                                fileEditor->document()->setModified(false);

                                if (newFile.error() != QFile::NoError) {
                                    QMessageBox::warning(
                                        this,
                                        tr(g_Messages[MessageKeys::g_MessageBoxFileReadErrorTitle]),
                                        tr(g_Messages[MessageKeys::g_MessageBoxFileReadErrorMessage]).arg(path)
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

    void TextEditor::onTabCloseRequested(int index) {
        this->closeFileAtIndex(index);
    }

    void TextEditor::onTabMoved(int from, int to) {
        Q_ASSERT(from < this->openFiles.size() && "The 'from' index is out of range.");
        Q_ASSERT(to < this->openFiles.size() && "The 'to' index is out of range.");

        this->openFiles.move(from, to);
    }

    void TextEditor::onRetranslateUI() {
        this->workingDirectory->setStatusTip(
            tr(g_Messages[MessageKeys::g_TooltipWorkingDirectoryView])
        );
    }
}
