#include "enriched_status_bar.h"

namespace CustomWidgets {
    void EnrichedStatusBar::onRetranslateUI() noexcept {
        this->setToolTips();

        elideText(
            this->statusToolTipLabel,
            this->savedToolTipMessage
                ? this->savedToolTipMessage->freeze()
                : QString{}
        );

        elideText(
            this->workingDirectoryLabel,
            this->savedWorkingDirectoryMessage
                ? this->savedWorkingDirectoryMessage->freeze()
                : QString{}
        );

        elideText(
            this->environmentStateLabel,
            this->savedEnvironmentStateMessage
                ? this->savedEnvironmentStateMessage->freeze()
                : QString{}
        );

        elideText(
            this->translatorResultLabel,
            this->savedTranslatorResultMessage
                ? this->savedTranslatorResultMessage->freeze()
                : QString{}
        );
    }

    bool EnrichedStatusBar::event(QEvent* event) {
        if (event->type() == QEvent::LanguageChange) {
            this->onRetranslateUI();
        }

        return QStatusBar::event(event);
    }
}
