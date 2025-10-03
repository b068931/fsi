#include <QStatusTipEvent>
#include <memory>

#include "main_window.h"
#include "qstring_wrapper.h"

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
}
