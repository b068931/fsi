#ifndef ENRICHED_STATUS_BAR_H
#define ENRICHED_STATUS_BAR_H

#include <QStatusBar>
#include <QLabel>
#include <QWidget>
#include <QString>

/// <summary>
/// A specialized instance of a QStatusBar that includes information about
/// the current working directory, state of the execution environment, and the last
/// result from the translator.
/// </summary>
class EnrichedStatusBar final : public QStatusBar
{
    Q_OBJECT
        
public:
    explicit EnrichedStatusBar(QWidget* parent = nullptr);

    EnrichedStatusBar(const EnrichedStatusBar&) = delete;
    EnrichedStatusBar& operator=(const EnrichedStatusBar&) = delete;

    EnrichedStatusBar(EnrichedStatusBar&&) = delete;
    EnrichedStatusBar& operator=(EnrichedStatusBar&&) = delete;

    ~EnrichedStatusBar() noexcept override;

public:
    enum class ColorHint {
        success,
        neutral,
        failure
    };

    /// <summary>
    /// Displays a tooltip with the specified message.
    /// </summary>
    /// <param name="message">The text to be shown in the tooltip.</param>
    void toolTip(const QString& message);

    /// <summary>
    /// Displays the working directory of the application.
    /// </summary>
    /// <param name="path">The path to the directory to set as the working directory.</param>
    void workingDirectory(const QString& path);

    /// <summary>
    /// Updates the state of the execution environment with a message and a color queue.
    /// </summary>
    /// <param name="message">A QString containing the message describing the environment state. For example, "running", "stopped", "crashed".</param>
    /// <param name="hint">A ColorQue object representing the color queue associated with the environment state.</param>
    void environmentState(const QString& message, ColorHint hint);

    /// <summary>
    /// Displays the result of translator execution with an associated color hint.
    /// </summary>
    /// <param name="message">The message string to display, the translation result. For example, "crashed", "failed", "success".</param>
    /// <param name="hint">A color hint indicating the status or type of the message.</param>
    void translatorResult(const QString& message, ColorHint hint);

private:
    // So as not to accidentally change status bar from
    // outside of this class.
    using QStatusBar::addPermanentWidget;
    using QStatusBar::addWidget;
    using QStatusBar::insertPermanentWidget;
    using QStatusBar::insertWidget;
    using QStatusBar::removeWidget;

private:
    QLabel* statusToolTipLabel;
    QLabel* workingDirectoryLabel;
    QLabel* environmentStateLabel;
    QLabel* translatorResultLabel;

    void setupUIComponents();
    void setVisualHintForLabel(QLabel* label, ColorHint hint);
};

#endif