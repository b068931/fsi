#include <QDir>
#include <QStatusBar>

#include "main_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), i18n(new Components::Internationalization::InterfaceTranslator{})
{
    this->ui.setupUi(this);

    this->setupStatusBar();
    this->setupTextEditor();
}

MainWindow::~MainWindow() noexcept = default;

void MainWindow::setupTextEditor() {
    Q_ASSERT(this->editor == nullptr && "The text editor has already been set up.");

    // We use central widget's layout because it has margins, while
    // QSplitter does not.
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

    this->editor = new CustomWidgets::TextEditor(centralWidget);
    centralLayout->addWidget(this->editor);

    QString defaultWorkingDirectory =  QDir::currentPath();
    this->editor->openWorkingDirectory(defaultWorkingDirectory);
    this->enrichedStatusBar->workingDirectory(defaultWorkingDirectory);
}

void MainWindow::setupStatusBar() {
    this->enrichedStatusBar = new CustomWidgets::EnrichedStatusBar(this);
    this->setStatusBar(this->enrichedStatusBar);

    //: Value for the status bar when no working directory is set.
    this->enrichedStatusBar->workingDirectory(
        tr("Working directory not set")
    );

    //: Value for the status bar when execution environment has not been started once yet.
    this->enrichedStatusBar->environmentState(
        tr("Not started"),
        CustomWidgets::EnrichedStatusBar::ColorHint::neutral
    );

    //: Value for the status bar when translator has not been run once yet.
    this->enrichedStatusBar->translatorResult(
        tr("Not started"),
        CustomWidgets::EnrichedStatusBar::ColorHint::neutral
    );
}

