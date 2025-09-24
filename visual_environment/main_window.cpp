#include <QDir>
#include "main_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->ui.setupUi(this);

    this->setupTextEditor();
    this->editor->openWorkingDirectory(QDir::currentPath());
}

MainWindow::~MainWindow() noexcept = default;

void MainWindow::setupTextEditor() {
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

    this->editor = new TextEditor(centralWidget);
    centralLayout->addWidget(this->editor);
}
