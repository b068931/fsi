#include <QDir>
#include <QStatusBar>

#include "main_window.h"

const QList<QLocale> MainWindow::supportedLocales = {
    QLocale(QLocale::Ukrainian, QLocale::Ukraine)
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->startupProbeLocales();
    this->ui.setupUi(this);

    this->setupStatusBar();
    this->setupTextEditor();
}

MainWindow::~MainWindow() noexcept {
    if (QTranslator* currentTranslator = this->activeTranslator.get()) {
        qApp->removeTranslator(currentTranslator);
    }
}

void MainWindow::loadTranslator(const QLocale* locale) {
    constexpr char resourcePath[] = ":/i18n";
    constexpr char resourcePrefix[] = "_";
    constexpr char fileName[] = "translation_visual_environment";
    constexpr char resourceSuffix[] = ".qm";

    if (QTranslator* currentTranslator = this->activeTranslator.get()) {
        qApp->removeTranslator(currentTranslator);
        this->activeTranslator.reset();
    }

    if (locale != nullptr) {
        this->activeTranslator.reset(new QTranslator{});
        if (this->activeTranslator->load(
            *locale, 
            fileName, 
            resourcePrefix, 
            resourcePath, 
            resourceSuffix)) {
            qApp->installTranslator(this->activeTranslator.get());
        }
        else {
            qWarning() << "Failed to load translator for locale:" << *locale;
        }
    }
}

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

    this->editor = new TextEditor(centralWidget);
    centralLayout->addWidget(this->editor);

    QString defaultWorkingDirectory =  QDir::currentPath();
    this->editor->openWorkingDirectory(defaultWorkingDirectory);
    this->enrichedStatusBar->workingDirectory(defaultWorkingDirectory);
}

void MainWindow::setupStatusBar() {
    this->enrichedStatusBar = new EnrichedStatusBar(this);
    this->setStatusBar(this->enrichedStatusBar);

    //: Value for the status bar when no working directory is set.
    this->enrichedStatusBar->workingDirectory(
        tr("Working directory not set")
    );

    //: Value for the status bar when execution environment has not been started once yet.
    this->enrichedStatusBar->environmentState(
        tr("Not started"),
        EnrichedStatusBar::ColorHint::neutral
    );

    //: Value for the status bar when translator has not been run once yet.
    this->enrichedStatusBar->translatorResult(
        tr("Not started"),
        EnrichedStatusBar::ColorHint::neutral
    );
}

void MainWindow::startupProbeLocales() {
    // If not supported, the application will run in English (default).
    QLocale systemLocale = QLocale::system();
    for (const QLocale& supportedLocale : MainWindow::supportedLocales) {
        if (supportedLocale.territory() == systemLocale.territory() && supportedLocale.language() == systemLocale.language()) {
            loadTranslator(&supportedLocale);
            return;
        }
    }
}
