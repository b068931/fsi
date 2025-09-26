#ifndef QT_VISUAL_ENVIRONMENT_MAIN_WINDOW_H
#define QT_VISUAL_ENVIRONMENT_MAIN_WINDOW_H

#include <QtGlobal>
#include <QtWidgets/QMainWindow>
#include <QString>
#include <QLabel>
#include <QEvent>
#include <QTranslator>
#include <QLocale>
#include <QList>

#include <functional>
#include <memory>

#include "ui_main_window.h"
#include "text_editor.h"
#include "enriched_status_bar.h"

/// <summary>
/// MainWindow provides the main access point
/// to the capabilities of the application. It manages
/// a simple text editor widget (for more information see text_editor.h)
/// and several child processes started on demand.
/// </summary>
class MainWindow final : public QMainWindow
{
    Q_OBJECT

private:
    static const QList<QLocale> supportedLocales;

public:
    explicit MainWindow(QWidget *parent = nullptr);

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator= (const MainWindow&) = delete;

    MainWindow(MainWindow&&) = delete;
    MainWindow& operator= (MainWindow&&) = delete;

    ~MainWindow() noexcept override;

protected:
    virtual bool event(QEvent* event) override;

public:
    /// <summary>
    /// Loads a translator resource by its name.
    /// Note that you must manually emit the retranslateUI() signal.
    /// </summary>
    /// <param name="locale">The name of the translator resource to load. Nullptr for default language (English).</param>
    void loadTranslator(const QLocale* locale);

signals:
    /// <summary>
    /// Signal emitted when the UI needs to be retranslated.
    /// For example, when the user changes the application language.
    /// </summary>
    void retranslateUI();

private:
    Ui::MainWindowClass ui{};

    std::unique_ptr<QTranslator> activeTranslator{};
    TextEditor* editor{};
    EnrichedStatusBar* enrichedStatusBar{};

    void setupTextEditor();
    void setupStatusBar();
    void startupProbeLocales();
};

#endif
