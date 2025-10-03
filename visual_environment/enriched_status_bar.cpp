#include <QVector>
#include <QString>
#include <QTGlobal>

#include "enriched_status_bar.h"
#include "enriched_status_bar_messages.h"

namespace {
    void setVisualHintForLabel(QLabel* label, CustomWidgets::EnrichedStatusBar::ColorHint hint) {
        Q_ASSERT(label && "Label is expected to be non-null.");
        QString styleSheetTemplate = QString(
            "QLabel {" \
            "   color: %1;" \
            "}"
        );

        switch (hint) {
        case CustomWidgets::EnrichedStatusBar::ColorHint::success: {
            styleSheetTemplate = styleSheetTemplate.arg("green");
            break;
        }
        case CustomWidgets::EnrichedStatusBar::ColorHint::neutral: {
            styleSheetTemplate = styleSheetTemplate.arg("white");
            break;
        }
        case CustomWidgets::EnrichedStatusBar::ColorHint::failure: {
            styleSheetTemplate = styleSheetTemplate.arg("red");
            break;
        }
        default:
            Q_ASSERT(false && "Unknown visual hint provided.");
        }

        label->setStyleSheet(styleSheetTemplate);
    }
}

namespace CustomWidgets {
    EnrichedStatusBar::EnrichedStatusBar(QWidget* parent)
        : QStatusBar{ parent }
    {
        this->setupUIComponents();
    }

    EnrichedStatusBar::~EnrichedStatusBar() noexcept = default;

    void EnrichedStatusBar::toolTip(
        std::unique_ptr<ITranslatableString> message
    ) {
        Q_ASSERT(message && "The message pointer must not be null.");
        Q_ASSERT(this->statusToolTipLabel && "The status tooltip label has not been set up.");

        this->savedToolTipMessage = std::move(message);
        this->statusToolTipLabel->setText(
            this->savedToolTipMessage->freeze()
        );
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void EnrichedStatusBar::workingDirectory(
        std::unique_ptr<ITranslatableString> path
    ) {
        Q_ASSERT(path && "The path pointer must not be null.");
        Q_ASSERT(this->workingDirectoryLabel && "The working directory label has not been set up.");

        this->savedWorkingDirectoryMessage = std::move(path);
        this->workingDirectoryLabel->setText(
            this->savedWorkingDirectoryMessage->freeze()
        );
    }

    void EnrichedStatusBar::environmentState(
        std::unique_ptr<ITranslatableString> message, 
        ColorHint hint
    ) {
        Q_ASSERT(message && "The message pointer must not be null.");
        Q_ASSERT(this->environmentStateLabel && "The environment state label has not been set up.");

        this->savedEnvironmentStateMessage = std::move(message);
        this->environmentStateLabel->setText(
            this->savedEnvironmentStateMessage->freeze()
        );

        setVisualHintForLabel(
            this->environmentStateLabel,
            hint
        );
    }

    void EnrichedStatusBar::translatorResult(
        std::unique_ptr<ITranslatableString> message, 
        ColorHint hint
    ) {
        Q_ASSERT(message && "The message pointer must not be null.");
        Q_ASSERT(this->translatorResultLabel && "The translator result label has not been set up.");

        this->savedTranslatorResultMessage = std::move(message);
        this->translatorResultLabel->setText(
            this->savedTranslatorResultMessage->freeze()
        );

        setVisualHintForLabel(
            this->translatorResultLabel,
            hint
        );
    }

    void EnrichedStatusBar::setupUIComponents() {
        Q_ASSERT(this->workingDirectoryLabel == nullptr && "The working directory label has already been set up.");
        Q_ASSERT(this->environmentStateLabel == nullptr && "The environment state label has already been set up.");
        Q_ASSERT(this->translatorResultLabel == nullptr && "The translator result label has already been set up.");

        this->statusToolTipLabel = new QLabel(this);
        this->workingDirectoryLabel = new QLabel(this);
        this->environmentStateLabel = new QLabel(this);
        this->translatorResultLabel = new QLabel(this);

        this->addPermanentWidget(this->statusToolTipLabel, 2);
        this->addPermanentWidget(this->translatorResultLabel, 1);
        this->addPermanentWidget(this->environmentStateLabel, 1);
        this->addPermanentWidget(this->workingDirectoryLabel, 0);

        this->statusToolTipLabel->setAlignment(Qt::AlignLeft);
        this->workingDirectoryLabel->setAlignment(Qt::AlignRight);
        this->environmentStateLabel->setAlignment(Qt::AlignCenter);
        this->translatorResultLabel->setAlignment(Qt::AlignCenter);

        this->setToolTips();

        // Margins so that text on the edges is not close to the window border.
        this->workingDirectoryLabel->setContentsMargins(0, 0, 5, 0);
        this->statusToolTipLabel->setContentsMargins(5, 0, 5, 0);
    }

    void EnrichedStatusBar::setToolTips() {
        Q_ASSERT(this->statusToolTipLabel && "The status tooltip label has not been set up.");
        Q_ASSERT(this->workingDirectoryLabel && "The working directory label has not been set up.");
        Q_ASSERT(this->environmentStateLabel && "The environment state label has not been set up.");
        Q_ASSERT(this->translatorResultLabel && "The translator result label has not been set up.");

        this->statusToolTipLabel->setToolTip(
            tr(g_Messages[MessageKeys::g_TooltipTooltips])
        );

        this->workingDirectoryLabel->setToolTip(
            tr(g_Messages[MessageKeys::g_TooltipWorkingDirectory])
        );

        this->translatorResultLabel->setToolTip(
            tr(g_Messages[MessageKeys::g_TooltipTranslatorResult])
        );

        this->environmentStateLabel->setToolTip(
            tr(g_Messages[MessageKeys::g_TooltipExecutionEnvironmentState])
        );
    }
}
