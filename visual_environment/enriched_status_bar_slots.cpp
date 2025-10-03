#include "enriched_status_bar.h"

namespace CustomWidgets {
    void EnrichedStatusBar::onRetranslateUI() {
        this->setToolTips();

        this->statusToolTipLabel->setText(
            this->savedToolTipMessage
                ? this->savedToolTipMessage->freeze()
                : QString{}
        );

        this->workingDirectoryLabel->setText(
            this->savedWorkingDirectoryMessage
                ? this->savedWorkingDirectoryMessage->freeze()
                : QString{}
        );

        this->environmentStateLabel->setText(
            this->savedEnvironmentStateMessage
                ? this->savedEnvironmentStateMessage->freeze()
                : QString{}
        );

        this->translatorResultLabel->setText(
            this->savedTranslatorResultMessage
                ? this->savedTranslatorResultMessage->freeze()
                : QString{}
        );
    }
}