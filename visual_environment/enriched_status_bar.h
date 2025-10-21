#ifndef ENRICHED_STATUS_BAR_H
#define ENRICHED_STATUS_BAR_H

#include <QStatusBar>
#include <QLabel>
#include <QWidget>
#include <QString>
#include <QEvent>

#include <memory>

#include "translatable_string_interface.h"

namespace CustomWidgets {
    /// <summary>
    /// A specialized instance of a QStatusBar that includes information about
    /// the current working directory, state of the execution environment, and the last
    /// result from the translator.
    /// </summary>
    class EnrichedStatusBar final : public QStatusBar
    {
        Q_OBJECT

    public:
        enum class ColorHint {
            success,
            neutral,
            failure
        };

        explicit EnrichedStatusBar(QWidget* parent = nullptr);
        ~EnrichedStatusBar() noexcept override;

        EnrichedStatusBar(const EnrichedStatusBar&) = delete;
        EnrichedStatusBar& operator=(const EnrichedStatusBar&) = delete;

        EnrichedStatusBar(EnrichedStatusBar&&) = delete;
        EnrichedStatusBar& operator=(EnrichedStatusBar&&) = delete;

        /// <summary>
        /// Displays a tooltip with the specified message.
        /// </summary>
        /// <param name="message">The text to be shown in the tooltip.</param>
        void toolTip(
            std::unique_ptr<Components::Internationalization::ITranslatableString> message
        );

        /// <summary>
        /// Displays the working directory of the application.
        /// </summary>
        /// <param name="path">The path to the directory to set as the working directory.</param>
        void workingDirectory(
            std::unique_ptr<Components::Internationalization::ITranslatableString> path
        );

        /// <summary>
        /// Updates the state of the execution environment with a message and a color queue.
        /// </summary>
        /// <param name="message">A QString containing the message describing the environment state. For example, "running", "stopped", "crashed".</param>
        /// <param name="hint">A ColorQue object representing the color queue associated with the environment state.</param>
        void environmentState(
            std::unique_ptr<Components::Internationalization::ITranslatableString> message, 
            ColorHint hint
        );

        /// <summary>
        /// Displays the result of translator execution with an associated color hint.
        /// </summary>
        /// <param name="message">The message string to display, the translation result. For example, "crashed", "failed", "success".</param>
        /// <param name="hint">A color hint indicating the status or type of the message.</param>
        void translatorResult(
            std::unique_ptr<Components::Internationalization::ITranslatableString> message, 
            ColorHint hint
        );

    public slots:
        void onRetranslateUI() noexcept;

    protected:
        virtual bool event(QEvent* event) override;

    private:
        using ITranslatableString = Components::Internationalization::ITranslatableString;

        // So as not to accidentally change status bar from
        // outside of this class.
        using QStatusBar::addPermanentWidget;
        using QStatusBar::addWidget;
        using QStatusBar::insertPermanentWidget;
        using QStatusBar::insertWidget;
        using QStatusBar::removeWidget;

        QLabel* statusToolTipLabel;
        QLabel* workingDirectoryLabel;
        QLabel* environmentStateLabel;
        QLabel* translatorResultLabel;

        std::unique_ptr<ITranslatableString> savedToolTipMessage;
        std::unique_ptr<ITranslatableString> savedWorkingDirectoryMessage;
        std::unique_ptr<ITranslatableString> savedEnvironmentStateMessage;
        std::unique_ptr<ITranslatableString> savedTranslatorResultMessage;

        static void setVisualHintForLabel(QLabel* label, ColorHint hint);
        static void elideText(QLabel* label, const QString& text);

        void setupUIComponents();
        void setToolTips();
    };
}

#endif