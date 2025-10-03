#include <QFileSystemModel>
#include <QFileInfo>
#include <QShowEvent>

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
