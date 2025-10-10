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
        // Connect the retranslateUI signal to the appropriate slots.
        connect(this->i18n.data(), &Components::Internationalization::InterfaceTranslator::retranslateUI,
            this, &MainWindow::onRetranslateUI);

        connect(this->i18n.data(), &Components::Internationalization::InterfaceTranslator::retranslateUI,
            this->editor, &CustomWidgets::TextEditor::onRetranslateUI);

        connect(this->i18n.data(), &Components::Internationalization::InterfaceTranslator::retranslateUI,
            this->enrichedStatusBar, &CustomWidgets::EnrichedStatusBar::onRetranslateUI);

        // Connect menu actions to their respective slots.
        connect(this->ui.actionUkrainian, &QAction::triggered,
            this, &MainWindow::onMenuLanguageUkrainian);

        connect(this->ui.actionEnglish, &QAction::triggered,
            this, &MainWindow::onMenuLanguageEnglish);

        connect(this->ui.openFolderMenuAction, &QAction::triggered,
            this, &MainWindow::onMenuWorkingDirectoryOpen);

        connect(this->ui.closeFolderMenuAction, &QAction::triggered,
            this, &MainWindow::onMenuWorkingDirectoryClose);

        connect(this->ui.newFileMenuAction, &QAction::triggered,
            this, &MainWindow::onMenuFileNew);

        connect(this->ui.openFileMenuAction, &QAction::triggered,
            this, &MainWindow::onMenuFileOpen);

        connect(this->ui.saveFileMenuAction, &QAction::triggered,
            this, &MainWindow::onMenuFileSave);

        connect(this->ui.saveFileAsMenuAction, &QAction::triggered,
            this, &MainWindow::onMenuFileSaveAs);

        connect(this->ui.closeFileMenuAction, &QAction::triggered,
            this, &MainWindow::onMenuFileClose);
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
    }

    void MainWindow::setupStatusBar() {
        this->enrichedStatusBar = new CustomWidgets::EnrichedStatusBar(this);
        this->setStatusBar(this->enrichedStatusBar);
    }
}

