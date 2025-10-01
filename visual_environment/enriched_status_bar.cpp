#include "enriched_status_bar.h"

namespace CustomWidgets {
    EnrichedStatusBar::EnrichedStatusBar(QWidget* parent)
        : QStatusBar{ parent }
    {
        this->setupUIComponents();
    }

    EnrichedStatusBar::~EnrichedStatusBar() noexcept = default;

    void EnrichedStatusBar::toolTip(const QString& message) {
        Q_ASSERT(this->statusToolTipLabel && "The status tooltip label has not been set up.");
        this->statusToolTipLabel->setText(message);
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void EnrichedStatusBar::workingDirectory(const QString& path) {
        Q_ASSERT(this->workingDirectoryLabel && "The working directory label has not been set up.");
        this->workingDirectoryLabel->setText(path);
    }

    void EnrichedStatusBar::environmentState(const QString& message, ColorHint hint) {
        Q_ASSERT(this->environmentStateLabel && "The environment state label has not been set up.");
        this->environmentStateLabel->setText(message);
        this->setVisualHintForLabel(
            this->environmentStateLabel,
            hint
        );
    }

    void EnrichedStatusBar::translatorResult(const QString& message, ColorHint hint) {
        Q_ASSERT(this->translatorResultLabel && "The translator result label has not been set up.");
        this->translatorResultLabel->setText(message);
        this->setVisualHintForLabel(
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

        //: Label in the status bar showing tooltips.
        this->statusToolTipLabel->setToolTip(
            tr("Tooltips")
        );

        //: Label in the status bar showing the working directory of the application.
        this->workingDirectoryLabel->setToolTip(
            tr("Working Directory")
        );

        //: Label in the status bar showing the result of translator execution.
        this->translatorResultLabel->setToolTip(
            tr("Translator Result")
        );

        //: Label in the status bar showing the state of the execution environment.
        this->environmentStateLabel->setToolTip(
            tr("Execution Environment State")
        );

        // Margins so that text on the edges is not close to the window border.
        this->workingDirectoryLabel->setContentsMargins(0, 0, 5, 0);
        this->statusToolTipLabel->setContentsMargins(5, 0, 5, 0);
    }

    void EnrichedStatusBar::setVisualHintForLabel(QLabel* label, ColorHint hint) {
        Q_ASSERT(label && "Label is expected to be non-null.");
        QString styleSheetTemplate = QString(
            "QLabel {" \
            "   color: %1;" \
            "}"
        );

        switch (hint) {
        case ColorHint::success: {
            styleSheetTemplate = styleSheetTemplate.arg("green");
            break;
        }
        case ColorHint::neutral: {
            styleSheetTemplate = styleSheetTemplate.arg("white");
            break;
        }
        case ColorHint::failure: {
            styleSheetTemplate = styleSheetTemplate.arg("red");
            break;
        }
        default:
            Q_ASSERT(false && "Unknown visual hint provided.");
        }

        label->setStyleSheet(styleSheetTemplate);
    }
}
