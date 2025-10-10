#include <QVBoxLayout>
#include <QWidget>
#include <QLayout>
#include <QFileSystemModel>
#include <QItemSelectionModel>
#include <QDir>
#include <QHeaderView>
#include <QModelIndex>
#include <QPlainTextEdit>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QTabBar>
#include <QMessageBox>
#include <QFileDialog>

#include <cstdlib>
#include <numeric>
#include <array>

#include "text_editor.h"
#include "text_editor_messages.h"

namespace CustomWidgets {
    QString TextEditor::OpenedFile::getNormalizedName() const {
        if (this->filePath.isEmpty()) {
            return tr(g_Messages[MessageKeys::g_TemporaryFileName]);
        }

        return this->filePath;
    }

    TextEditor::TextEditor(QWidget* parent)
        : QWidget{ parent }, fileWatcher{ new QFileSystemWatcher(this) }
    {
        this->setupEditorComponents();
        this->setupWorkingDirectoryContextMenu();
        this->connectSignalsManually();
    }

    TextEditor::~TextEditor() noexcept = default;

    void TextEditor::connectSignalsManually() {
        Q_ASSERT(this->workingDirectory != nullptr && "The working directory view has not been set up.");
        Q_ASSERT(this->fileTabs != nullptr && "The file tabs have not been set up.");
        Q_ASSERT(this->fileWatcher && "The file watcher has not been set up.");

        // Connect signals for objects which were created manually
        // at application startup.
        connect(this->workingDirectory, &QTreeView::doubleClicked,
            this, &TextEditor::onWorkingDirectoryItemDoubleClicked);

        connect(this->workingDirectory, &QTreeView::customContextMenuRequested,
            this, &TextEditor::onWorkingDirectoryContextMenu);

        connect(this->fileTabs, &QTabWidget::tabCloseRequested,
            this, &TextEditor::onTabCloseRequested);

        connect(this->fileTabs->tabBar(), &QTabBar::tabMoved,
            this, &TextEditor::onTabMoved);

        connect(this->fileWatcher, &QFileSystemWatcher::fileChanged,
            this, &TextEditor::onFileChangedOutside);
    }

    void TextEditor::setupEditorComponents() {
        Q_ASSERT(this->splitter == nullptr && "The splitter has already been set up.");
        Q_ASSERT(this->workingDirectory == nullptr && "The working directory view has already been set up.");
        Q_ASSERT(this->fileTabs == nullptr && "The file tabs have already been set up.");

        // The layout won't contain anything else, apart from the splitter.
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        this->setLayout(mainLayout);

        this->splitter = new QSplitter(Qt::Horizontal, this);
        this->workingDirectory = new QTreeView(this->splitter);
        this->fileTabs = new QTabWidget(this->splitter);

        this->splitter->addWidget(this->workingDirectory);
        this->splitter->addWidget(this->fileTabs);
        mainLayout->addWidget(this->splitter);

        // Finish setting up the components by adjusting their properties.
        mainLayout->setContentsMargins(0, 0, 0, 0);

        this->fileTabs->setTabsClosable(true);
        this->fileTabs->setMovable(true);
        this->fileTabs->setDocumentMode(true);
        this->fileTabs->setContextMenuPolicy(Qt::CustomContextMenu);

        this->workingDirectory->setUniformRowHeights(true);
        this->workingDirectory->setAnimated(true);
        this->workingDirectory->setContextMenuPolicy(Qt::CustomContextMenu);

        this->workingDirectory->setStatusTip(
            tr(g_Messages[MessageKeys::g_TooltipWorkingDirectoryView])
        );

    }

    void TextEditor::setupWorkingDirectoryContextMenu() {
        Q_ASSERT(this->workingDirectory != nullptr && "The working directory has not been set up.");

        // Setup the context menu for the working directory
        this->workingDirectoryContextMenu = new QMenu(this);
        this->openAction = this->workingDirectoryContextMenu->addAction(tr(g_Messages[MessageKeys::g_ContextMenuOpen]));
        this->workingDirectoryContextMenu->addSeparator();
        this->newFileAction = this->workingDirectoryContextMenu->addAction(tr(g_Messages[MessageKeys::g_ContextMenuNewFile]));
        this->newDirectoryAction = this->workingDirectoryContextMenu->addAction(tr(g_Messages[MessageKeys::g_ContextMenuNewDirectory]));
        this->workingDirectoryContextMenu->addSeparator();
        this->removeAction = this->workingDirectoryContextMenu->addAction(tr(g_Messages[MessageKeys::g_ContextMenuRemove]));
    }

    void TextEditor::setDefaultSplitterRatio() {
        Q_ASSERT(this->splitter != nullptr && "The splitter has not been set up.");

        // We want the text editor to be larger than the
        // information about working directory.

        constexpr int workingDirectoryParts = 1;
        constexpr int textEditorParts = 6;
        constexpr int partsList[]{ workingDirectoryParts, textEditorParts };
        constexpr int totalParts = std::accumulate(partsList, partsList + std::size(partsList), 0);

        QList<int> newSizes(std::size(partsList));
        int totalSize = this->splitter->width();
        for (qsizetype index = 0; index < newSizes.size(); ++index) {
            newSizes[index] = partsList[index] * (totalSize / totalParts);
        }

        this->splitter->setSizes(newSizes);
    }

    bool TextEditor::closeAllFiles() {
        constexpr int firstElement = 0;
        if (this->openFiles.isEmpty()) {
            return true;
        }

        qsizetype previousCount;
        qsizetype newCount = this->openFiles.size();
        do {
            // Keep removing the first element up until user cancels the operation, or we run out of files.
            previousCount = newCount;
            this->closeFileAtIndex(firstElement);
            newCount = this->openFiles.size();
        } while (newCount < previousCount && !this->openFiles.isEmpty());

        return this->openFiles.isEmpty();
    }

    bool TextEditor::hasSelectedFile() const noexcept {
        return this->fileTabs->currentIndex() != -1;
    }

    void TextEditor::openNewFile(const QString& filePath) {
        Q_ASSERT(!filePath.isEmpty() && "The provided file path is empty.");
        Q_ASSERT(this->fileTabs != nullptr && "The file tabs have not been set up.");

        try {
            QFileInfo fileInfo(filePath);
            if (!fileInfo.isFile()) {
                throw tr(g_Messages[MessageKeys::g_MessageBoxFileOpenErrorMessage]).arg(filePath);
            }

            QString absoluteFilePath = fileInfo.absoluteFilePath();
            for (int index = 0; index < this->openFiles.size(); ++index) {
                if (this->openFiles[index].filePath == absoluteFilePath) {
                    this->fileTabs->setCurrentIndex(index);
                    return;
                }
            }

            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                throw tr(g_Messages[MessageKeys::g_MessageBoxFileOpenErrorMessage]).arg(filePath);
            }

            if (!this->fileWatcher->addPath(absoluteFilePath)) {
                throw tr(g_Messages[MessageKeys::g_MessageBoxFileOpenErrorMessage]).arg(filePath);
            }

            QPlainTextEdit* fileEditor = new QPlainTextEdit(this);
            fileEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
            fileEditor->setPlainText(file.readAll());
            fileEditor->document()->setModified(false);

            if (file.error() != QFile::NoError) {
                throw tr(g_Messages[MessageKeys::g_MessageBoxFileReadErrorMessage]).arg(filePath);
            }

            this->openFiles.append(
                {
                    .filePath = absoluteFilePath,
                    .isTemporary = false
                }
            );

            const int newTabIndex = this->fileTabs->addTab(fileEditor, fileInfo.fileName());
            this->fileTabs->setCurrentIndex(newTabIndex);
            this->fileTabs->setTabToolTip(newTabIndex, absoluteFilePath);
        }
        catch (const QString& message) {
            // This is not a critical error, so we can just show a message box.
            // It is not really a good practice to throw exceptions which are not derived
            // from std::exception, but creating a custom class for this isolated case is
            // not worth the effort.

            QMessageBox::warning(
                this,
                tr(g_Messages[MessageKeys::g_MessageBoxFileOpenErrorTitle]),
                message
            );
        }
    }

    void TextEditor::createTemporaryFile() {
        QPlainTextEdit* fileEditor = new QPlainTextEdit(this);
        fileEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
        fileEditor->setPlainText("");
        fileEditor->document()->setModified(false);

        this->openFiles.append(
            {
                .filePath = QString(),
                .isTemporary = true
            }
        );

        const int newTabIndex = this->fileTabs->addTab(fileEditor, tr(g_Messages[MessageKeys::g_TemporaryFileName]));
        this->fileTabs->setCurrentIndex(newTabIndex);
        this->fileTabs->setTabToolTip(newTabIndex, tr(g_Messages[MessageKeys::g_TemporaryFileName]));
    }

    bool TextEditor::saveCurrentFile() {
        if (this->fileTabs->currentIndex() == -1) 
            return false;

        return this->saveFileAtIndex(
            this->fileTabs->currentIndex()
        );
    }

    bool TextEditor::saveCurrentFileAs() {
        if (this->fileTabs->currentIndex() == -1) 
            return false;

        int currentIndex = this->fileTabs->currentIndex();
        OpenedFile& openFile = this->openFiles[currentIndex];

        // Try to save the file as if it was temporary, and didn't have a save path.
        bool savedTemporaryStatus = openFile.isTemporary;
        QString savedFilePath = std::move(
            openFile.filePath
        );

        openFile.isTemporary = true;
        bool result = this->saveFileAtIndex(currentIndex);

        // The file is still temporary, so the user must have cancelled the save operation.
        // Restore the previous state.
        if (openFile.isTemporary) {
            openFile.isTemporary = savedTemporaryStatus;
            openFile.filePath = std::move(savedFilePath);
        }

        return result;
    }

    bool TextEditor::closeCurrentFile() {
        if (this->fileTabs->currentIndex() == -1) 
            return false;

        return this->closeFileAtIndex(
            this->fileTabs->currentIndex()
        );
    }

    bool TextEditor::closeFileAtIndex(int index) {
        Q_ASSERT(this->fileTabs != nullptr && "The file tabs have not been set up.");
        Q_ASSERT(this->fileTabs->count() == this->openFiles.count() && "The file tabs and open files count do not match.");
        Q_ASSERT(index < this->fileTabs->count() && index >= 0 && "The specified index is out of range.");

        OpenedFile& openFile = this->openFiles[index];
        QPlainTextEdit* fileEditor = this->getEditorAtIndex(index);
        if (fileEditor->document()->isModified()) {
            QMessageBox::StandardButton result = QMessageBox::question(
                this,
                tr(g_Messages[MessageKeys::g_MessageBoxFileCloseConfirmationTitle]),
                tr(g_Messages[MessageKeys::g_MessageBoxFileCloseConfirmationMessage]).arg(
                    openFile.getNormalizedName()
                ),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                QMessageBox::Yes
            );

            if (result == QMessageBox::Yes) {
                // Discard the return value, as we can't do anything if the save gets cancelled.
                this->saveFileAtIndex(index);
            }
            else if (result == QMessageBox::No) {
                this->fileTabs->removeTab(index);
                this->openFiles.removeAt(index);

                if (!openFile.filePath.isEmpty() && !openFile.isTemporary) {
                    if (!this->fileWatcher->removePath(openFile.filePath)) {
                        qWarning() << "Failed to remove file from the file watcher at path:" << openFile.filePath;
                    }
                }

                return true;
            }
            else {
                // The user closed the message box without making a choice.
                // Do nothing and return.
                return false;
            }
        }

        if (!fileEditor->document()->isModified()) {
            this->fileTabs->removeTab(index);
            this->openFiles.removeAt(index);

            if (!openFile.filePath.isEmpty() && !openFile.isTemporary) {
                if (!this->fileWatcher->removePath(openFile.filePath)) {
                    qWarning() << "Failed to remove file from the file watcher at path:" << openFile.filePath;
                }
            }

            return true;
        }

        return false;
    }

    bool TextEditor::saveFileAtIndex(int index) {
        Q_ASSERT(this->fileTabs && "The file tabs have not been set up.");
        Q_ASSERT(this->fileTabs->count() == this->openFiles.count() && "The file tabs and open files count do not match.");
        Q_ASSERT(index < this->fileTabs->count() && index >= 0 && "The specified index is out of range.");

        OpenedFile& openFile = this->openFiles[index];
        QPlainTextEdit* fileEditor = this->getEditorAtIndex(index);

        if (fileEditor->document()->isModified() && !openFile.isTemporary) {
            QFile oldFile(openFile.filePath);
            if (oldFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                fileEditor->document()->setModified(false);

                QByteArray buffer = fileEditor->toPlainText().toUtf8();
                qint64 bytesWritten = oldFile.write(buffer);
                if (bytesWritten != buffer.size() || oldFile.error() != QFile::NoError) {
                    fileEditor->document()->setModified(true);
                    QMessageBox::warning(
                        this,
                        tr(g_Messages[MessageKeys::g_MessageBoxFileWriteErrorTitle]),
                        tr(g_Messages[MessageKeys::g_MessageBoxFileWriteErrorMessage]).arg(
                            openFile.getNormalizedName()
                        )
                    );

                    return false;
                }

                oldFile.close();
                return true;
            }
            else {
                fileEditor->document()->setModified(true);
                openFile.isTemporary = true;

                QMessageBox::warning(
                    this,
                    tr(g_Messages[MessageKeys::g_MessageBoxFileSaveErrorTitle]),
                    tr(g_Messages[MessageKeys::g_MessageBoxFileSaveErrorMessage]).arg(
                        openFile.getNormalizedName()
                    )
                );

                return false;
            }
        }
        else if (openFile.isTemporary) {
            // The file is temporary, so we need to ask the user where to save it.
            // Notice that the default directory is either the working directory or
            // the last known directory of the file.

            QString filePath = QFileDialog::getSaveFileName(
                this,
                tr(g_Messages[MessageKeys::g_SaveFileDialogTitle]),
                openFile.filePath.isEmpty() ? this->getWorkingDirectoryPath() : openFile.filePath,
                tr(g_Messages[MessageKeys::g_SaveFileDialogFilter])
            );

            if (!filePath.isEmpty()) {
                QFile newFile(filePath);
                if (newFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QFileInfo fileInfo(filePath);

                    openFile.filePath = fileInfo.absoluteFilePath();
                    fileEditor->document()->setModified(false);
                    openFile.isTemporary = false;

                    this->fileTabs->setTabText(index, fileInfo.fileName());
                    this->fileTabs->setTabToolTip(index, fileInfo.absoluteFilePath());

                    QByteArray buffer = fileEditor->toPlainText().toUtf8();
                    qint64 bytesWritten = newFile.write(buffer);
                    if (bytesWritten != buffer.size() || newFile.error() != QFile::NoError) {
                        fileEditor->document()->setModified(true);
                        openFile.isTemporary = true;

                        QMessageBox::warning(
                            this,
                            tr(g_Messages[MessageKeys::g_MessageBoxFileWriteErrorTitle]),
                            tr(g_Messages[MessageKeys::g_MessageBoxFileWriteErrorMessage]).arg(
                                fileInfo.absoluteFilePath()
                            )
                        );

                        return false;
                    }
                    else {
                        // Don't watch for file changes if the write fails. Assume that the file is inaccessible to the user.
                        if (!this->fileWatcher->addPath(fileInfo.absoluteFilePath())) {
                            qWarning() << "Failed to add newly saved file to the file watcher at path :" << fileInfo.absoluteFilePath();
                        }
                    }

                    newFile.close();
                    return true;
                }
                else {
                    QMessageBox::warning(
                        this,
                        tr(g_Messages[MessageKeys::g_MessageBoxFileSaveErrorTitle]),
                        tr(g_Messages[MessageKeys::g_MessageBoxFileSaveErrorMessage]).arg(
                            openFile.getNormalizedName()
                        )
                    );

                    return false;
                }
            }
        }

        return false;
    }

    QPlainTextEdit* TextEditor::getEditorAtIndex(int index) {
        QPlainTextEdit* fileEditor = qobject_cast<QPlainTextEdit*>(this->fileTabs->widget(index));
        if (fileEditor == nullptr) {
            // Remove a possibly corrupted tab and its associated data.
            this->fileTabs->removeTab(index);
            this->openFiles.removeAt(index);

            // Then try to salvage other open files by closing all of them.
            this->closeAllFiles();
            qFatal() << "Failed to cast the file editor widget to QPlainTextEdit." << __LINE__ << __FILE__;

            return nullptr;
        }

        return fileEditor;
    }

    void TextEditor::openWorkingDirectory(const QString& directoryPath) {
        Q_ASSERT(this->workingDirectory != nullptr && "The working directory view has not been set up.");

        if (QItemSelectionModel* oldModel = this->workingDirectory->selectionModel()) {
            oldModel->deleteLater();
        }

        this->workingDirectoryPath = directoryPath;
        if (!directoryPath.isEmpty()) {
            QFileSystemModel* newModel = new QFileSystemModel{};
            QModelIndex rootDirectory = newModel->setRootPath(directoryPath);
            newModel->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

            // Set new model and hide all columns except "Name".
            // "Name" column has zero index.

            this->workingDirectory->header()->hide();
            this->workingDirectory->setModel(newModel);
            this->workingDirectory->setRootIndex(rootDirectory);
            for (int index = 1; index < newModel->columnCount(); ++index) {
                this->workingDirectory->hideColumn(index);
            }
        }
        else {
            this->workingDirectory->setModel(nullptr);
        }
    }

    const QString& TextEditor::getWorkingDirectoryPath() const noexcept {
        return this->workingDirectoryPath;
    }
}
