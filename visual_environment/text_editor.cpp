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
    TextEditor::TextEditor(QWidget* parent)
        : QWidget{ parent }, fileWatcher{ new QFileSystemWatcher(this) }
    {
        this->setupEditorComponents();
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

    void TextEditor::closeAllFiles() {
        for (int index = 0; index < this->openFiles.size(); ++index) {
            this->closeFileAtIndex(index);
        }
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

            QPlainTextEdit* fileEditor = new QPlainTextEdit;
            fileEditor->setPlainText(file.readAll());

            this->openFiles.append(
                {
                    .filePath = absoluteFilePath,
                    .isTemporary = false
                }
            );

            const int newTabIndex = this->fileTabs->addTab(fileEditor, fileInfo.fileName());
            this->fileTabs->setCurrentIndex(newTabIndex);
            this->fileTabs->setTabToolTip(newTabIndex, absoluteFilePath);
            this->fileTabs->widget(newTabIndex)->setStatusTip(absoluteFilePath);
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

    void TextEditor::closeFileAtIndex(int index) {
        Q_ASSERT(this->fileTabs != nullptr && "The file tabs have not been set up.");
        Q_ASSERT(this->fileTabs->count() == this->openFiles.count() && "The file tabs and open files count do not match.");
        Q_ASSERT(index < this->fileTabs->count() && index >= 0 && "The specified index is out of range.");

        // TODO: Prompt the user with the "Are you sure you want to close without saving?" dialog before calling saveFileAtIndex or closing it without saving.

        OpenedFile& openFile = this->openFiles[index];
        QPlainTextEdit* fileEditor = qobject_cast<QPlainTextEdit*>(this->fileTabs->widget(index));

        this->saveFileAtIndex(index);
        this->fileTabs->removeTab(index);
        this->openFiles.remove(index);

        if (!this->fileWatcher->removePath(openFile.filePath)) {
            qWarning() << "Failed to remove file from the file watcher at path :" << openFile.filePath;
        }
    }

    void TextEditor::saveFileAtIndex(int index) {
        Q_ASSERT(this->fileTabs && "The file tabs have not been set up.");
        Q_ASSERT(index < this->fileTabs->count() && index >= 0 && "The specified index is out of range.");

        OpenedFile& openFile = this->openFiles[index];
        QPlainTextEdit* fileEditor = qobject_cast<QPlainTextEdit*>(this->fileTabs->widget(index));
        if (fileEditor == nullptr) {
            this->closeAllFiles();
            qFatal() << "Failed to cast the file editor widget to QPlainTextEdit." << __LINE__ << __FILE__;
        }

        if (fileEditor->document()->isModified() && !openFile.isTemporary) {
            QFile oldFile(openFile.filePath);
            if (oldFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                fileEditor->document()->setModified(false);
                oldFile.write(fileEditor->toPlainText().toUtf8());
                oldFile.close();
            }
            else {
                openFile.isTemporary = true;
                QMessageBox::warning(
                    this,
                    tr(g_Messages[MessageKeys::g_MessageBoxFileSaveErrorTitle]),
                    tr(g_Messages[MessageKeys::g_MessageBoxFileSaveErrorMessage]).arg(openFile.filePath)
                );
            }
        }
        else if (openFile.isTemporary) {
            // TODO: Check if a file already has filePath set. This can happen when file was removed from the outside of the text editor, for example.

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
                    this->fileTabs->widget(index)->setStatusTip(fileInfo.absoluteFilePath());

                    // TODO: Implement additional to check to ensure that the file was fully written.
                    // TODO: Handle the case when the file was not fully written.
                    // TODO: Check whether readAll function can fail and handle that case.

                    newFile.write(fileEditor->toPlainText().toUtf8());
                    newFile.close();

                    if (!this->fileWatcher->addPath(fileInfo.absoluteFilePath())) {
                        qWarning() << "Failed to add newly saved file to the file watcher at path :" << fileInfo.absoluteFilePath();
                    }
                }
                else {
                    QMessageBox::warning(
                        this,
                        tr(g_Messages[MessageKeys::g_MessageBoxFileSaveErrorTitle]),
                        tr(g_Messages[MessageKeys::g_MessageBoxFileSaveErrorMessage]).arg(openFile.filePath)
                    );
                }
            }
        }
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
