#include <QStatusTipEvent>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QMetaObject>

#include <memory>

#include "main_window.h"
#include "qstring_wrapper.h"
#include "main_window_messages.h"
#include "static_translatable_string.h"
#include "fsi_ui_configuration_options.h"

namespace Windows {
    bool MainWindow::event(QEvent* event) {
        if (event->type() == QEvent::StatusTip) {
            QStatusTipEvent* statusTipEvent = dynamic_cast<QStatusTipEvent*>(event);
            Q_ASSERT(statusTipEvent != nullptr && "Unexpected: the event is not of type QStatusTipEvent.");

            QString statusTip = statusTipEvent->tip();
            if (!statusTip.isEmpty()) {
                this->enrichedStatusBar->toolTip(
                    Components::Internationalization::QStringWrapper::wrap(std::move(statusTip))
                );
            }

            return true;
        }

        if (event->type() == QEvent::LanguageChange) {
            this->onRetranslateUI();
        }

        return QMainWindow::event(event);
    }

    void MainWindow::closeEvent(QCloseEvent* event) {
        Q_ASSERT(this->editor && "The text editor has not been set up.");
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");

        if (this->editor->closeAllFiles()) {
            QMainWindow::closeEvent(event);
        }
        else {
            event->ignore();
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipCloseCanceled]
                )
            );
        }
    }

    void MainWindow::showEvent(QShowEvent* event) {
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");
        Q_ASSERT(this->editor && "The editor has not been set up.");

        if (!event->spontaneous()) {
            QString defaultWorkingDirectory = QDir::currentPath();
            this->editor->openWorkingDirectory(defaultWorkingDirectory);
            this->enrichedStatusBar->workingDirectory(
                Components::Internationalization::QStringWrapper::wrap(
                    std::move(defaultWorkingDirectory)
                )
            );

            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusTipStartup]
                )
            );

            this->enrichedStatusBar->environmentState(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusBarDefaultEnvironmentState]
                ),
                CustomWidgets::EnrichedStatusBar::ColorHint::neutral
            );

            this->enrichedStatusBar->translatorResult(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusBarDefaultTranslatorState]
                ),
                CustomWidgets::EnrichedStatusBar::ColorHint::neutral
            );
        }

        QMainWindow::showEvent(event);
    }

    void MainWindow::onMenuFileNew() noexcept {
        Q_ASSERT(this->editor && "The text editor has not been set up.");
        this->editor->createTemporaryFile();
    }

    void MainWindow::onMenuFileOpen() noexcept {
        Q_ASSERT(this->editor && "The text editor has not been set up.");

        QString filePath = QFileDialog::getOpenFileName(
            this,
            tr(g_Messages[g_DialogTitleOpenFile]),
            this->editor->getWorkingDirectoryPath(),
            tr(g_Messages[g_DialogFilterSourceFiles])
        );

        // Only open the file if a valid path was selected.
        if (!filePath.isNull() && !filePath.isEmpty()) {
            this->editor->openNewFile(filePath);
        }
    }

    void MainWindow::onMenuFileSave() noexcept {
        Q_ASSERT(this->editor && "The text editor has not been set up.");
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");
        if (!this->editor->hasSelectedFile()) {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipNoFileSelected]
                )
            );

            return;
        }

        if (this->editor->saveCurrentFile()) {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipFileSaveSucceeded]
                )
            );
        }
        else {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipFileSaveCanceled]
                )
            );
        }
    }

    void MainWindow::onMenuFileSaveAs() noexcept {
        Q_ASSERT(this->editor && "The text editor has not been set up.");
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");
        if (!this->editor->hasSelectedFile()) {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipNoFileSelected]
                )
            );

            return;
        }

        if (this->editor->saveCurrentFileAs()) {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipFileSaveSucceeded]
                )
            );
        }
        else {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipFileSaveCanceled]
                )
            );
        }
    }

    void MainWindow::onMenuFileClose() noexcept {
        Q_ASSERT(this->editor && "The text editor has not been set up.");
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");
        if (!this->editor->hasSelectedFile()) {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipNoFileSelected]
                )
            );

            return;
        }

        if (!this->editor->closeCurrentFile()) {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipFileCloseCanceled]
                )
            );
        }
    }

    void MainWindow::onRetranslateUI() noexcept {
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");

        this->ui.retranslateUi(this);
        this->enrichedStatusBar->toolTip(
            Components::Internationalization::StaticTranslatableString::wrap(
                g_Context,
                g_Messages[g_StatusTipLanguageChanged]
            )
        );
    }

    void MainWindow::onMenuLanguageUkrainian() noexcept {
        Q_ASSERT(this->i18n && "The internationalization component has not been set up.");
        this->i18n->setLanguage(
            Components::Internationalization::InterfaceTranslator::Language::Ukrainian
        );
    }

    void MainWindow::onMenuLanguageEnglish() noexcept {
        Q_ASSERT(this->i18n && "The internationalization component has not been set up.");
        this->i18n->setLanguage(
            Components::Internationalization::InterfaceTranslator::Language::English
        );
    }

    void MainWindow::onMenuChangeThemeDark() noexcept {
        Q_ASSERT(this->applicationStyle && "The application style manager has not been set up.");
        if (this->applicationStyle->setStyle(Components::ApplicationStyle::ApplicationStylesManager::Style::Dark)) {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipThemeChangedSuccessfully]
                )
            );
        }
        else {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipThemeChangeFailed]
                )
            );
        }
    }

    void MainWindow::onMenuChangeThemeLight() noexcept {
        Q_ASSERT(this->applicationStyle && "The application style manager has not been set up.");
        if (this->applicationStyle->setStyle(Components::ApplicationStyle::ApplicationStylesManager::Style::Light)) {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipThemeChangedSuccessfully]
                )
            );
        }
        else {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipThemeChangeFailed]
                )
            );
        }
    }

    void MainWindow::onMenuShortDescription() noexcept {
        Q_ASSERT(this->aboutWindow && "The about window has not been set up.");
        this->aboutWindow->show();
    }

    void MainWindow::onMenuWorkingDirectoryOpen() noexcept {
        Q_ASSERT(this->editor && "The text editor has not been set up.");
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");

        QString newWorkingDirectory = QFileDialog::getExistingDirectory(
            this,
            tr(g_Messages[g_DialogTitleOpenWorkingDirectory]),
            QDir::homePath(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );

        if (!newWorkingDirectory.isEmpty()) {
            this->editor->openWorkingDirectory(newWorkingDirectory);
            this->enrichedStatusBar->workingDirectory(
                Components::Internationalization::QStringWrapper::wrap(
                    std::move(newWorkingDirectory)
                )
            );
        }
        else {
           this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipWorkingDirectorySelectionCanceled]
                )
           );
        }
    }

    void MainWindow::onMenuWorkingDirectoryClose() noexcept {
        Q_ASSERT(this->editor && "The text editor has not been set up.");
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");

        this->editor->openWorkingDirectory(QString());
        this->enrichedStatusBar->workingDirectory(
            std::make_unique<Components::Internationalization::StaticTranslatableString>(
                g_Context,
                g_Messages[g_StatusBarDefaultWorkingDirectory]
            )
        );
    }

    void MainWindow::onProgramTranslate() noexcept {
        Q_ASSERT(this->editor && "The text editor has not been set up.");
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");

        if (!this->editor->hasSelectedFile()) {
           this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipNoFileSelected]
                )
            );

           return;
        }

        // Ensure the current file is saved before translation.
        // If the user cancels the save operation, the old version of the file is used.
        // Automatically handles cases where the file is not modified.
        if (!this->editor->saveCurrentFile()) {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[g_StatusTipTranslationWithoutSave]
                )
            );
        }
        else {
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusTipFileSavedBeforeTranslation]
                )
            );
        }

        QString selectedFile = this->editor->getCurrentFilePath();
        QString translationResult = QDir(this->editor->getWorkingDirectoryPath())
            .filePath(g_Messages[MessageKeys::g_TranslationResultFileName]);

        if (selectedFile.isEmpty()) {
            QMessageBox::warning(
                this,
                tr(g_Messages[MessageKeys::g_DialogTitleUnsavedTemporary]),
                tr(g_Messages[MessageKeys::g_DialogMessageUnsavedTemporary])
            );

            return;
        }

        auto debugFlag = 
            Components::FSITools::ConfigurationOptions::getTranslatorDebugFlag(this);

        if (debugFlag.has_value()) {
            this->languageService->send(
                [selectedFile, translationResult, debugFlag]
                (Components::FSITools::FSIToolsAdapter* adapter) {
                    adapter->startProgramTranslation(
                        selectedFile,
                        translationResult,
                        debugFlag.value()
                    );
                });
        }
        else {
            this->enrichedStatusBar->translatorResult(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusTipProgramTranslationCancelled]
                ),
                CustomWidgets::EnrichedStatusBar::ColorHint::neutral
            );
        }
    }

    void MainWindow::onRunLastTranslatedProgram() noexcept {
        Q_ASSERT(this->editor && "The text editor has not been set up.");
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");

        QString logFilePath = QDir(this->editor->getWorkingDirectoryPath())
            .filePath(g_Messages[MessageKeys::g_EELogFileName]);

        QString translationResult = QDir(this->editor->getWorkingDirectoryPath())
            .filePath(g_Messages[MessageKeys::g_TranslationResultFileName]);

        std::optional<QString> configurationFilePath =
            Components::FSITools::ConfigurationOptions::getExecutionEnvironmentConfiguration(this);

        if (configurationFilePath.has_value()) {
            this->languageService->send(
                [configurationFilePath, translationResult, logFilePath]
                (Components::FSITools::FSIToolsAdapter* adapter) {
                    adapter->startExecutionEnvironment(
                        configurationFilePath.value(),
                        Components::FSITools::ConfigurationOptions::getPreferredNumberOfExecutors(),
                        translationResult,
                        logFilePath
                    );
                }
            );
        }
        else {
            this->enrichedStatusBar->environmentState(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusTipProgramExecutionCancelled]
                ),
                CustomWidgets::EnrichedStatusBar::ColorHint::neutral
            );
        }
    }

    void MainWindow::onTranslateAndRun() noexcept {
        this->runAfterTranslation = true;
        this->onProgramTranslate();
    }

    void MainWindow::onRuntimeEnvironmentLogs() noexcept {
        Q_ASSERT(this->editor && "The text editor has not been set up.");

        QString logFilePath = QDir(this->editor->getWorkingDirectoryPath())
            .filePath(g_Messages[MessageKeys::g_EELogFileName]);

        // Editor will automatically produce an error message if the file does not exist.
        this->editor->openNewFile(logFilePath);
    }

    void MainWindow::onProgramTranslatorStarted() noexcept {
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");

        this->enrichedStatusBar->translatorResult(
            Components::Internationalization::StaticTranslatableString::wrap(
                g_Context,
                g_Messages[MessageKeys::g_StatusTipTranslatorStarted]
            ),
            CustomWidgets::EnrichedStatusBar::ColorHint::neutral
        );
    }

    void MainWindow::onProgramTranslationResult(
        int exitCode,
        Components::FSITools::FSIToolsAdapter::ChildResult result
    ) noexcept {
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");

        // Run after translation flag is always reset. However, the program is run only if
        // the translation was successful.
        bool savedRunAfterTranslationFlag = this->runAfterTranslation;
        this->runAfterTranslation = false;

        switch (result) {
        case Components::FSITools::FSIToolsAdapter::ChildResult::terminated: {
            // 0 indicates a successful exit.
            if (exitCode) {
                this->enrichedStatusBar->translatorResult(
                    Components::Internationalization::StaticTranslatableString::wrap(
                        g_Context,
                        g_Messages[MessageKeys::g_StatusTipTranslatorProgramHasErrors]
                    ),
                    CustomWidgets::EnrichedStatusBar::ColorHint::failure
                );
            }
            else {
                this->enrichedStatusBar->translatorResult(
                    Components::Internationalization::StaticTranslatableString::wrap(
                        g_Context,
                        g_Messages[MessageKeys::g_StatusTipTranslatorSuccess]
                    ),
                    CustomWidgets::EnrichedStatusBar::ColorHint::success
                );

                if (savedRunAfterTranslationFlag) {
                    this->onRunLastTranslatedProgram();
                }
            }

            break;
        }

        case Components::FSITools::FSIToolsAdapter::ChildResult::failedToStart: {
            this->enrichedStatusBar->translatorResult(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusTipTranslatorFailedToStart]
                ),
                CustomWidgets::EnrichedStatusBar::ColorHint::failure
            );

            break;
        }

        case Components::FSITools::FSIToolsAdapter::ChildResult::crashed: {
            this->enrichedStatusBar->translatorResult(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusTipTranslatorCrashed]
                ),
                CustomWidgets::EnrichedStatusBar::ColorHint::failure
            );

            QMessageBox::warning(
            this,
                tr(g_Messages[MessageKeys::g_DialogTitleTranslatorCrashed]),
                tr(g_Messages[MessageKeys::g_DialogMessageTranslatorCrashed])
            );

            break;
        }

        case Components::FSITools::FSIToolsAdapter::ChildResult::alreadyRunning: {
            this->enrichedStatusBar->translatorResult(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusTipTranslatorAlreadyRunning]
                ),
                CustomWidgets::EnrichedStatusBar::ColorHint::failure
            );

            QString lastTranslationTarget = this->languageService->send<QString>(
                [] (const Components::FSITools::FSIToolsAdapter* adapter) {
                    return adapter->getLastTranslationTarget();
                }
            );

            QString lastTranslationResult = this->languageService->send<QString>(
                [] (const Components::FSITools::FSIToolsAdapter* adapter) {
                    return adapter->getLastTranslationResult();
                }
            );

            QMessageBox::warning(
                this,
                tr(g_Messages[MessageKeys::g_DialogTitleTranslatorAlreadyRunning]),
                tr(g_Messages[MessageKeys::g_DialogMessageTranslatorAlreadyRunning])
                    .arg(lastTranslationTarget)
                    .arg(lastTranslationResult)
            );

            break;
        }

        case Components::FSITools::FSIToolsAdapter::ChildResult::killedByUser: {
            this->enrichedStatusBar->translatorResult(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusTipInstanceKilledByUser]
                ),
                CustomWidgets::EnrichedStatusBar::ColorHint::neutral
            );

            break;
        }

        case Components::FSITools::FSIToolsAdapter::ChildResult::unknownError: {
            QMessageBox::warning(
                this,
                tr(g_Messages[MessageKeys::g_DialogTitleUnknownResult]),
                tr(g_Messages[MessageKeys::g_DialogMessageUnknownResult])
            );

            break;
        }
        }
    }

    void MainWindow::onExecutionEnvironmentStarted() noexcept {
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");

        this->enrichedStatusBar->environmentState(
            Components::Internationalization::StaticTranslatableString::wrap(
                g_Context,
                g_Messages[MessageKeys::g_StatusTipTranslatorStarted]
            ),
            CustomWidgets::EnrichedStatusBar::ColorHint::neutral
        );
    }

    void MainWindow::onExecutionEnvironmentResult(
        int exitCode,
        Components::FSITools::FSIToolsAdapter::ChildResult result
    ) noexcept {
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");

        switch (result) {
        case Components::FSITools::FSIToolsAdapter::ChildResult::terminated: {
            // 0 indicates a successful exit.
            if (exitCode) {
                this->enrichedStatusBar->environmentState(
                    Components::Internationalization::StaticTranslatableString::wrap(
                        g_Context,
                        g_Messages[MessageKeys::g_StatusTipExecutionEnvironmentProgramHasErrors]
                    ),
                    CustomWidgets::EnrichedStatusBar::ColorHint::failure
                );
            }
            else {
                this->enrichedStatusBar->environmentState(
                    Components::Internationalization::StaticTranslatableString::wrap(
                        g_Context,
                        g_Messages[MessageKeys::g_StatusTipExecutionEnvironmentSuccess]
                    ),
                    CustomWidgets::EnrichedStatusBar::ColorHint::success
                );
            }

            break;
        }

        case Components::FSITools::FSIToolsAdapter::ChildResult::failedToStart: {
            this->enrichedStatusBar->environmentState(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusTipExecutionEnvironmentFailedToStart]
                ),
                CustomWidgets::EnrichedStatusBar::ColorHint::failure
            );

            break;
        }

        case Components::FSITools::FSIToolsAdapter::ChildResult::crashed: {
            this->enrichedStatusBar->environmentState(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusTipExecutionEnvironmentCrashed]
                ),
                CustomWidgets::EnrichedStatusBar::ColorHint::failure
            );

            QMessageBox::warning(
                this,
                tr(g_Messages[MessageKeys::g_DialogTitleExecutionEnvironmentCrashed]),
                tr(g_Messages[MessageKeys::g_DialogMessageExecutionEnvironmentCrashed])
            );

            break;
        }

        case Components::FSITools::FSIToolsAdapter::ChildResult::alreadyRunning: {
            this->enrichedStatusBar->environmentState(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusTipExecutionEnvironmentAlreadyRunning]
                ),
                CustomWidgets::EnrichedStatusBar::ColorHint::failure
            );

            QString lastExecutionEnvironmentProgram = this->languageService->send<QString>(
                [] (const Components::FSITools::FSIToolsAdapter* adapter) {
                    return adapter->getLastExecutionEnvironmentProgram();
                }
            );

            QString lastExecutionEnvironmentConfiguration = this->languageService->send<QString>(
                [] (const Components::FSITools::FSIToolsAdapter* adapter) {
                    return adapter->getLastExecutionEnvironmentConfiguration();
                }
            );

            QMessageBox::warning(
                this,
                tr(g_Messages[MessageKeys::g_DialogTitleExecutionEnvironmentAlreadyRunning]),
                tr(g_Messages[MessageKeys::g_DialogMessageExecutionEnvironmentAlreadyRunning])
                    .arg(lastExecutionEnvironmentProgram)
                    .arg(lastExecutionEnvironmentConfiguration)
            );

            break;
        }

        case Components::FSITools::FSIToolsAdapter::ChildResult::killedByUser: {
            this->enrichedStatusBar->environmentState(
                Components::Internationalization::StaticTranslatableString::wrap(
                    g_Context,
                    g_Messages[MessageKeys::g_StatusTipInstanceKilledByUser]
                ),
                CustomWidgets::EnrichedStatusBar::ColorHint::neutral
            );

            break;
        }

        case Components::FSITools::FSIToolsAdapter::ChildResult::unknownError: {
            QMessageBox::warning(
                this,
                tr(g_Messages[MessageKeys::g_DialogTitleUnknownResult]),
                tr(g_Messages[MessageKeys::g_DialogMessageUnknownResult])
            );

            break;
        }
        }
    }

}
