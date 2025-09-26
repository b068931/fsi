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

#include <cstdlib>
#include <numeric>
#include <array>

#include "text_editor.h"

TextEditor::TextEditor(QWidget* parent)
    :QWidget{ parent }
{
    this->setupEditorComponents();
    this->connectSignalsManually();
}

TextEditor::~TextEditor() noexcept = default;

void TextEditor::connectSignalsManually() {
    Q_ASSERT(this->workingDirectory != nullptr && "The working directory view has not been set up.");
    Q_ASSERT(this->fileTabs != nullptr && "The file tabs have not been set up.");

    // Connect signals for objects which were created manually
    // at application startup.
    connect(this->workingDirectory, &QTreeView::doubleClicked, 
        this, &TextEditor::onWorkingDirectoryItemDoubleClicked);
    
    connect(this->fileTabs, &QTabWidget::tabCloseRequested,
        this, &TextEditor::onTabCloseRequested);

    connect(this->fileTabs->tabBar(), &QTabBar::tabMoved, 
        this, &TextEditor::onTabMoved);
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

    //: Status tip for the working directory view in the text editor.
    this->workingDirectory->setStatusTip(
        tr("Working directory view. Use double-click or context menu.")
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

void TextEditor::openNewFile(const QString& filePath) {
    Q_ASSERT(this->fileTabs != nullptr && "The file tabs have not been set up.");

    for (int index = 0; index < this->openFiles.size(); ++index) {
        if (this->openFiles[index].filePath == filePath) {
            this->fileTabs->setCurrentIndex(index);
            return;
        }
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //: Status tip for the error message box when a file cannot be opened.
        QMessageBox::warning(this, tr("Error"),
            tr("Could not open file \"%1\" for reading.").arg(filePath));
        return;
    }

    QPlainTextEdit* fileEditor = new QPlainTextEdit;
    fileEditor->setPlainText(file.readAll());

    QFileInfo fileInfo(filePath);
    this->openFiles.append(
        {
            .filePath = filePath,
            .isTemporary = false
        }
    );

    const int newTabIndex = this->fileTabs->addTab(fileEditor, fileInfo.fileName());
    this->fileTabs->setCurrentIndex(newTabIndex);
    this->fileTabs->setTabToolTip(newTabIndex, filePath);
    this->fileTabs->widget(newTabIndex)->setStatusTip(filePath);
}

void TextEditor::closeFileAtIndex(int index) {
    Q_ASSERT(this->fileTabs != nullptr && "The file tabs have not been set up.");
    Q_ASSERT(this->fileTabs->count() == this->openFiles.count() && "The file tabs and open files count do not match.");
    Q_ASSERT(index < this->fileTabs->count() && index >= 0 && "The specified index is out of range.");

    this->fileTabs->removeTab(index);
    this->openFiles.remove(index);
}

void TextEditor::openWorkingDirectory(const QString& directoryPath) {
    Q_ASSERT(this->workingDirectory != nullptr && "The working directory view has not been set up.");

    if (QItemSelectionModel* oldModel = this->workingDirectory->selectionModel()) {
        oldModel->deleteLater();
    }

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
