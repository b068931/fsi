#include <QDir>
#include <QVector>
#include <QPair>
#include <QApplication>

#include "main_window.h"
#include "qstring_wrapper.h"
#include "static_translatable_string.h"
#include "main_window_messages.h"

namespace Windows {
    MainWindow::MainWindow(
        AboutApplicationWindow* aboutWindow,
        Utility::BackgroundService<Components::FSITools::FSIToolsAdapter>* languageService,
        Components::Internationalization::InterfaceTranslator* i18n,
        Components::ApplicationStyle::ApplicationStylesManager* applicationStyle,
        QWidget* parent
    )
        : QMainWindow(parent),
          aboutWindow(aboutWindow),
          languageService(languageService),
          i18n(i18n),
          applicationStyle(applicationStyle)
    {
        this->ui.setupUi(this);

        this->setupStatusBar();
        this->setupTextEditor();
        this->configureDocumentation();
        this->connectSignalsManually();
    }

    MainWindow::~MainWindow() noexcept = default;

    void MainWindow::connectSignalsManually() {
        Q_ASSERT(this->languageService && "The language service has not been set up.");

        // Connect FSIToolsAdapter signals to their respective slots.
        this->languageService->receive(
            &Components::FSITools::FSIToolsAdapter::translatorStarted,
            this,
            &MainWindow::onProgramTranslatorStarted
        );

        this->languageService->receive(
            &Components::FSITools::FSIToolsAdapter::translationResult,
            this,
            &MainWindow::onProgramTranslationResult
        );

        this->languageService->receive(
            &Components::FSITools::FSIToolsAdapter::executionEnvironmentStarted,
            this,
            &MainWindow::onExecutionEnvironmentStarted
        );

        this->languageService->receive(
            &Components::FSITools::FSIToolsAdapter::executionEnvironmentResult,
            this,
            &MainWindow::onExecutionEnvironmentResult
        );

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

        connect(this->ui.translateOnlyMenuAction, &QAction::triggered, 
            this, &MainWindow::onProgramTranslate);

        connect(this->ui.runLastTranslatedMenuAction, &QAction::triggered, 
            this, &MainWindow::onRunLastTranslatedProgram);

        connect(this->ui.translateAndRunMenuAction, &QAction::triggered, 
            this, &MainWindow::onTranslateAndRun);

        connect(this->ui.executionEnvironmentMenuAction, &QAction::triggered,
            this, &MainWindow::onRuntimeEnvironmentLogs);

        connect(this->ui.shortDescriptionMenuAction, &QAction::triggered,
            this, &MainWindow::onMenuShortDescription);

        connect(this->ui.changeThemeDarkAction, &QAction::triggered,
            this, &MainWindow::onMenuChangeThemeDark);

        connect(this->ui.changeThemeLightAction, &QAction::triggered,
            this, &MainWindow::onMenuChangeThemeLight);
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

    void MainWindow::configureDocumentation() {
        QVector<QPair<QAction*, QString>> documentationCorrelationPaths{
            { this->ui.visualEnvironmentDocumentationMenuAction,
                "visual-environment.txt"},

            { this->ui.documentationFSIEngineMultithreadingMenuAction,
                "engine-multithreading.txt"},

            { this->ui.documentationFSIInstallationMenuAction,
                "installation.txt"},

            { this->ui.documentationFSIReleaseLayoutMenuAction,
                "release-layout.txt"},

            { this->ui.documentationFSIUseExecutionEnvironmentMenuAction,
                "use-fsi-mediator.txt"},

            { this->ui.documentationFSIUseTranslatorMenuAction,
                "use-fsi-translator.txt"}
        };

        // Just open all documentation files in editor in read-only mode.
        // If a file can't be modified, then text editor just won't do anything with it.
        for (auto& [action, path] : documentationCorrelationPaths) {
            connect(action, &QAction::triggered, this, [this, path]() {
                constexpr bool readOnlyMode = true;
                auto buildPathToFile = [](const QString& fileName) -> QString {
                    // Convert path relative to the application binary path into
                    // absolute system path.
                    return QDir(QApplication::applicationDirPath()).filePath(
                        tr(g_Messages[g_DocumentationBasePath]).arg(fileName));
                };

                if (this->editor->openNewFile(buildPathToFile(path), readOnlyMode)) {
                    this->enrichedStatusBar->toolTip(
                        Components::Internationalization::StaticTranslatableString::wrap(
                            g_Context,
                            g_Messages[g_StatusTipDocumentationOpened]
                        )
                    );
                }
            });
        }
    }
}

