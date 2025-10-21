#include "about_application_window.h"

namespace Windows {
    void AboutApplicationWindow::onRetranslateUI() noexcept {
        this->ui.retranslateUi(this);
    }

    bool AboutApplicationWindow::event(QEvent* event) {
        if (event->type() == QEvent::LanguageChange) {
            this->onRetranslateUI();
        }

        return QWidget::event(event);
    }
}