#ifndef QT_VISUAL_ENVIRONMENT_MAIN_WINDOW_H
#define QT_VISUAL_ENVIRONMENT_MAIN_WINDOW_H

#include <QTreeView>
#include <QTabWidget>
#include <QtWidgets/QMainWindow>
#include <QString>

#include "ui_main_window.h"

/// <summary>
/// MainWindow is a text editor. You are supposed to use
/// it in order to quickly edit programs and launch them
/// in the execution environment.
/// </summary>
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator= (const MainWindow&) = delete;

    MainWindow(MainWindow&&) = delete;
    MainWindow& operator= (MainWindow&&) = delete;

    ~MainWindow() noexcept override;

private slots:
    void onWorkingDirectoryItemDoubleClicked(const QModelIndex& index);

private:
    Ui::MainWindowClass ui{};

    QTabWidget* textEditor{};
    QTreeView* workingDirectory{};

    void connectSignalsManually();
    void setupEditorComponents();
    void openWorkingDirectory(const QString& directoryPath);
};

#endif
