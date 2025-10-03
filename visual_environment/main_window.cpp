#include <QDir>

#include "main_window.h"
#include "qstring_wrapper.h"
#include "static_translatable_string.h"
#include "main_window_messages.h"

namespace Windows {
    MainWindow::MainWindow(QWidget* parent)
        : QMainWindow(parent), i18n(new Components::Internationalization::InterfaceTranslator{})
    {
        this->ui.setupUi(this);

        this->setupStatusBar();
        this->setupTextEditor();
        this->connectSignalsManually();
    }

    MainWindow::~MainWindow() noexcept = default;

    void MainWindow::connectSignalsManually() {
        connect(this->i18n.data(), &Components::Internationalization::InterfaceTranslator::retranslateUI,
            this, &MainWindow::onRetranslateUI);

        connect(this->i18n.data(), &Components::Internationalization::InterfaceTranslator::retranslateUI,
            this->editor, &CustomWidgets::TextEditor::onRetranslateUI);

        connect(this->i18n.data(), &Components::Internationalization::InterfaceTranslator::retranslateUI,
            this->enrichedStatusBar, &CustomWidgets::EnrichedStatusBar::onRetranslateUI);

        connect(this->ui.actionUkrainian, &QAction::triggered,
            this, &MainWindow::onMenuLanguageUkrainian);

        connect(this->ui.actionEnglish, &QAction::triggered,
            this, &MainWindow::onMenuLanguageEnglish);
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

        this->editor = new CustomWidgets::TextEditor(centralWidget);
        centralLayout->addWidget(this->editor);

        QString defaultWorkingDirectory = QDir::currentPath();
        this->editor->openWorkingDirectory(defaultWorkingDirectory);
        this->enrichedStatusBar->workingDirectory(
            Components::Internationalization::QStringWrapper::wrap(
                std::move(defaultWorkingDirectory)
            )
        );
    }

    void MainWindow::setupStatusBar() {
        this->enrichedStatusBar = new CustomWidgets::EnrichedStatusBar(this);
        this->setStatusBar(this->enrichedStatusBar);

        //: Value for the status bar when no working directory is set.
        this->enrichedStatusBar->workingDirectory(
            Components::Internationalization::StaticTranslatableString::wrap(
                g_Context,
                g_Messages[MessageKeys::g_StatusBarDefaultWorkingDirectory]
            )
        );

        //: Value for the status bar when execution environment has not been started once yet.
        this->enrichedStatusBar->environmentState(
            Components::Internationalization::StaticTranslatableString::wrap(
                g_Context,
                g_Messages[MessageKeys::g_StatusBarDefaultEnvironmentState]
            ),
            CustomWidgets::EnrichedStatusBar::ColorHint::neutral
        );

        //: Value for the status bar when translator has not been run once yet.
        this->enrichedStatusBar->translatorResult(
            Components::Internationalization::StaticTranslatableString::wrap(
                g_Context,
                g_Messages[MessageKeys::g_StatusBarDefaultTranslatorState]
            ),
            CustomWidgets::EnrichedStatusBar::ColorHint::neutral
        );
    }
}

