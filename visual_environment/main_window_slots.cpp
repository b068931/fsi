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
        if (this->editor->closeAllFiles()) {
            QMainWindow::closeEvent(event);
        }
        else {
            event->ignore();
            this->enrichedStatusBar->toolTip(
                Components::Internationalization::QStringWrapper::wrap(
                    tr(g_Messages[g_StatusTipCloseCanceled])
                )
            );
        }
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
        this->editor->saveCurrentFile();
    }

    void MainWindow::onMenuFileSaveAs() {
        Q_ASSERT(this->editor && "The text editor has not been set up.");
        this->editor->saveCurrentFileAs();
    }

    void MainWindow::onMenuFileClose() {
        Q_ASSERT(this->editor && "The text editor has not been set up.");
        this->editor->closeCurrentFile();
    }

    void MainWindow::onRetranslateUI() {
        this->ui.retranslateUi(this);
        this->enrichedStatusBar->toolTip(
            Components::Internationalization::QStringWrapper::wrap(
                tr(g_Messages[g_StatusTipLanguageChanged])
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

        this->editor->openWorkingDirectory(newWorkingDirectory);
        this->enrichedStatusBar->workingDirectory(
            Components::Internationalization::QStringWrapper::wrap(
                std::move(newWorkingDirectory)
            )
        );
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
