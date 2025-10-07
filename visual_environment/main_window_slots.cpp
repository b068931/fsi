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

            this->enrichedStatusBar->toolTip(
                Components::Internationalization::QStringWrapper::wrap(
                    statusTipEvent->tip()
                )
            );

            return true;
        }

        return QMainWindow::event(event);
    }

    void MainWindow::closeEvent(QCloseEvent* event) {
        this->editor->closeAllFiles();
        QMainWindow::closeEvent(event);
    }

    void MainWindow::onMenuFileOpen() {
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

    void MainWindow::onRetranslateUI() {
        this->ui.retranslateUi(this);
    }

    void MainWindow::onMenuLanguageUkrainian() {
        this->i18n->setLanguage(
            Components::Internationalization::InterfaceTranslator::Language::Ukrainian
        );
    }

    void MainWindow::onMenuLanguageEnglish() {
        this->i18n->setLanguage(
            Components::Internationalization::InterfaceTranslator::Language::English
        );
    }

    void MainWindow::onMenuWorkingDirectoryOpen() {
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
        this->editor->openWorkingDirectory(QString());
        this->enrichedStatusBar->workingDirectory(
            std::make_unique<Components::Internationalization::StaticTranslatableString>(
                g_Context,
                g_Messages[g_StatusBarDefaultWorkingDirectory]
            )
        );
    }
}
