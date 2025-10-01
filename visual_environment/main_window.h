#ifndef QT_VISUAL_ENVIRONMENT_MAIN_WINDOW_H
#define QT_VISUAL_ENVIRONMENT_MAIN_WINDOW_H

#include <QEvent>
#include <QtGlobal>
#include <QtWidgets/QMainWindow>
#include <QString>

#include <memory>

#include "ui_main_window.h"
#include "text_editor.h"
#include "enriched_status_bar.h"
#include "interface_translator.h"

/// <summary>
/// MainWindow provides the main access point
/// to the capabilities of the application. It manages
/// a simple text editor widget (for more information see text_editor.h)
/// and several child processes started on demand.
/// Essentially, this class implements the mediator pattern.
/// </summary>
class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() noexcept override;

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator= (const MainWindow&) = delete;

    MainWindow(MainWindow&&) = delete;
    MainWindow& operator= (MainWindow&&) = delete;

protected:
    virtual bool event(QEvent* event) override;

private:
    Ui::MainWindowClass ui{};
    std::unique_ptr<Components::Internationalization::InterfaceTranslator> i18n{};

    CustomWidgets::TextEditor* editor{};
    CustomWidgets::EnrichedStatusBar* enrichedStatusBar{};

    void setupTextEditor();
    void setupStatusBar();
};

#endif
