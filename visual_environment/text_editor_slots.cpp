#include <QFileSystemModel>
#include <QFileInfo>
#include <QShowEvent>
#include <QMessageBox>
#include <QPlainTextEdit>

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
        Q_ASSERT(this->fileWatcher && "The file watcher has not been set up.");

        // Start tracking the file again if it was removed from the watcher for some reason.
        if (!this->fileWatcher->files().contains(path)) {
            this->fileWatcher->addPath(path);
        }

        QFileInfo fileInfo(path);
        for (int index = 0; index < this->openFiles.size(); ++index) {
            OpenedFile& openFile = this->openFiles[index];
            if (openFile.filePath == path) {
                if (!fileInfo.exists()) {
                    QMessageBox::StandardButton result = QMessageBox::question(
                        this,
                        tr(g_Messages[MessageKeys::g_MessageBoxFileRemovedTitle]),
                        tr(g_Messages[MessageKeys::g_MessageBoxFileRemovedMessage]).arg(path),
                        QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::No
                    );

                    if (result == QMessageBox::Yes) {
                        // Remove the tab and stop tracking the file.
                        // Do not prompt to save changes, as the file is gone.
                        this->fileTabs->removeTab(index);
                        this->openFiles.removeAt(index);
                        this->fileWatcher->removePath(path);
                    }
                    else {
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

                        openFile.isTemporary = true;
                        if (!this->fileWatcher->removePath(path)) {
                            qWarning() << "Failed to remove path from file watcher:" << path;
                        }
                    }
                    else {
                        QPlainTextEdit* fileEditor = qobject_cast<QPlainTextEdit*>(this->fileTabs->widget(index));
                        if (fileEditor == nullptr) {
                            this->closeAllFiles();
                            qFatal() << "Failed to cast tab widget to QPlainTextEdit." << __LINE__ << __FILE__;
                        }

                        if (!fileEditor->document()->isModified() && !openFile.isTemporary) {
                            // This will get executed even when we modify and save the file ourselves.
                            fileEditor->setPlainText(newFile.readAll());
                        }
                        else {
                            QMessageBox::StandardButton result = QMessageBox::question(
                                this,
                                tr(g_Messages[MessageKeys::g_MessageBoxFileChangedOutsideTitle]),
                                tr(g_Messages[MessageKeys::g_MessageBoxFileChangedOutsideMessage]).arg(path),
                                QMessageBox::Yes | QMessageBox::No,
                                QMessageBox::No
                            );

                            if (result == QMessageBox::Yes) {
                                fileEditor->setPlainText(newFile.readAll());
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
