#include <QTGlobal>
#include <QFontMetrics>
#include <QString>

#include "enriched_status_bar.h"
#include "enriched_status_bar_messages.h"

namespace {
}

namespace CustomWidgets {
    EnrichedStatusBar::EnrichedStatusBar(QWidget* parent)
        : QStatusBar{ parent },
        statusToolTipLabel{ nullptr },
        workingDirectoryLabel{ nullptr },
        environmentStateLabel{ nullptr },
        translatorResultLabel{ nullptr }
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
        elideText(
            this->statusToolTipLabel,
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
        elideText(
            this->workingDirectoryLabel,
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
        elideText(
            this->environmentStateLabel,
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
        elideText(
            this->translatorResultLabel,
            this->savedTranslatorResultMessage->freeze()
        );

        setVisualHintForLabel(
            this->translatorResultLabel,
            hint
        );
    }

    void EnrichedStatusBar::setVisualHintForLabel(QLabel* label, CustomWidgets::EnrichedStatusBar::ColorHint hint) {
        Q_ASSERT(label && "Label is expected to be non-null.");
        constexpr const char successColor[]{ "#38ff38" };
        constexpr const char failureColor[]{ "#ff3838" };
        constexpr const char neutralColor[]{ "#757575" };

        QString styleSheetTemplate = QString(
            "QLabel {" \
            "   color: %1;" \
            "}"
        );

        switch (hint) {
        case CustomWidgets::EnrichedStatusBar::ColorHint::success: {
            styleSheetTemplate = styleSheetTemplate.arg(successColor);
            break;
        }
        case CustomWidgets::EnrichedStatusBar::ColorHint::neutral: {
            styleSheetTemplate = styleSheetTemplate.arg(neutralColor);
            break;
        }
        case CustomWidgets::EnrichedStatusBar::ColorHint::failure: {
            styleSheetTemplate = styleSheetTemplate.arg(failureColor);
            break;
        }
        default:
            Q_ASSERT(false && "Unknown visual hint provided.");
        }

        label->setStyleSheet(styleSheetTemplate);
    }

    void EnrichedStatusBar::elideText(QLabel* label, const QString& text) {
        Q_ASSERT(label && "Label is expected to be non-null.");

        QFontMetrics fontMetrics = label->fontMetrics();
        QString elidedText = fontMetrics.elidedText(
            text,
            Qt::ElideMiddle,
            label->width() - label->contentsMargins().left() - label->contentsMargins().right()
        );

        label->setText(elidedText);
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
        this->addPermanentWidget(this->workingDirectoryLabel, 2);

        this->statusToolTipLabel->setAlignment(Qt::AlignCenter);
        this->workingDirectoryLabel->setAlignment(Qt::AlignCenter);
        this->environmentStateLabel->setAlignment(Qt::AlignCenter);
        this->translatorResultLabel->setAlignment(Qt::AlignCenter);

        this->setToolTips();

        // Margins so that text on the edges is not close to the window border.
        this->statusToolTipLabel->setContentsMargins(5, 0, 5, 0);
        this->workingDirectoryLabel->setContentsMargins(5, 0, 5, 0);
        this->environmentStateLabel->setContentsMargins(5, 0, 5, 0);
        this->translatorResultLabel->setContentsMargins(5, 0, 5, 0);
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
