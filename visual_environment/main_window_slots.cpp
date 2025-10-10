#include <QStatusTipEvent>
#include <QFileDialog>
#include <QDir>
#include <memory>

#include "main_window.h"
#include "qstring_wrapper.h"
#include "main_window_messages.h"
#include "static_translatable_string.h"

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

    void MainWindow::onMenuFileNew() {
        Q_ASSERT(this->editor && "The text editor has not been set up.");
        this->editor->createTemporaryFile();
    }

    void MainWindow::onMenuFileOpen() {
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

    void MainWindow::onMenuFileSave() {
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

    void MainWindow::onMenuFileSaveAs() {
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

    void MainWindow::onMenuFileClose() {
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

    void MainWindow::onRetranslateUI() {
        Q_ASSERT(this->enrichedStatusBar && "The status bar has not been set up.");

        this->ui.retranslateUi(this);
        this->enrichedStatusBar->toolTip(
            Components::Internationalization::StaticTranslatableString::wrap(
                g_Context,
                g_Messages[g_StatusTipLanguageChanged]
            )
        );
    }

    void MainWindow::onMenuLanguageUkrainian() {
        Q_ASSERT(this->i18n && "The internationalization component has not been set up.");
        this->i18n->setLanguage(
            Components::Internationalization::InterfaceTranslator::Language::Ukrainian
        );
    }

    void MainWindow::onMenuLanguageEnglish() {
        Q_ASSERT(this->i18n && "The internationalization component has not been set up.");
        this->i18n->setLanguage(
            Components::Internationalization::InterfaceTranslator::Language::English
        );
    }

    void MainWindow::onMenuWorkingDirectoryOpen() {
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

    void MainWindow::onMenuWorkingDirectoryClose() {
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
}
