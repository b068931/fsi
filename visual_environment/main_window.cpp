#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QLayout>
#include <QFileSystemModel>
#include <QItemSelectionModel>
#include <QDir>
#include <QHeaderView>
#include <QModelIndex>

#include <cstdlib>
#include <numeric>
#include <array>

#include "main_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->ui.setupUi(this);
    this->setupEditorComponents();

    this->openWorkingDirectory(QDir::currentPath());
}

MainWindow::~MainWindow() noexcept = default;

void MainWindow::connectSignalsManually() {
    // Connect signals for objects which were created manually
    // at application startup.
    connect(this->workingDirectory, &QTreeView::doubleClicked, 
        this, &MainWindow::onWorkingDirectoryItemDoubleClicked);
}

void MainWindow::setupEditorComponents() {
    // We use central widget's layout because it has margins, while
    // QSplitter does not. QTabWidget is called a textEditor
    // because it will contain QPlainTextEdits.

    QWidget* centralWidget = this->findChild<QWidget*>("centralWidget");
    if (centralWidget == nullptr) {
        QWidget* defaultCentralWidget = new QWidget();
        this->setCentralWidget(defaultCentralWidget);

        centralWidget = defaultCentralWidget;
    }

    QLayout* centralLayout = centralWidget->layout();
    if (centralLayout == nullptr) {
        constexpr int defaultMargin = 9;
        QVBoxLayout* defaultLayout = new QVBoxLayout(centralWidget);

        centralWidget->setLayout(defaultLayout);
        defaultLayout->setContentsMargins(defaultMargin, defaultMargin, defaultMargin, defaultMargin);

        centralLayout = defaultLayout;
    }

    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    this->workingDirectory = new QTreeView(splitter);
    this->textEditor = new QTabWidget(splitter);


    splitter->addWidget(this->workingDirectory);
    splitter->addWidget(this->textEditor);

    centralLayout->addWidget(splitter);

    // We want the text editor to be larger than the
    // information about working directory.
    constexpr int workingDirectoryParts = 1;
    constexpr int textEditorParts = 6;
    constexpr int partsList[]{ workingDirectoryParts, textEditorParts };
    constexpr int totalParts = std::accumulate(partsList, partsList + std::size(partsList), 0);

    QList<int> newSizes(std::size(partsList));
    int totalSize = splitter->width();
    for (qsizetype index = 0; index < newSizes.size(); ++index) {
        newSizes[index] = partsList[index] * (totalSize / totalParts);
    }

    splitter->setSizes(newSizes);
}

void MainWindow::openWorkingDirectory(const QString& directoryPath) {
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
