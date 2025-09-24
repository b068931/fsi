#ifndef QT_VISUAL_ENVIRONMENT_MAIN_WINDOW_H
#define QT_VISUAL_ENVIRONMENT_MAIN_WINDOW_H

#include <QtWidgets/QMainWindow>

#include "text_editor.h"
#include "ui_main_window.h"

/// <summary>
/// MainWindow provides the main access point
/// to the capabilities of the application. It manages
/// a simple text editor widget (for more information see text_editor.h)
/// and a runtime environment.
/// </summary>
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator= (const MainWindow&) = delete;

    MainWindow(MainWindow&&) = delete;
    MainWindow& operator= (MainWindow&&) = delete;

    ~MainWindow() noexcept override;

private:
    Ui::MainWindowClass ui{};
    TextEditor* editor{};

    void setupTextEditor();
};

#endif
