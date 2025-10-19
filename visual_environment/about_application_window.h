#ifndef ABOUT_APPLICATION_WINDOW_H
#define ABOUT_APPLICATION_WINDOW_H

#include <QDialog>
#include "ui_about_application_window.h"

class AboutApplicationWindow : public QWidget {
    Q_OBJECT

public:
    explicit AboutApplicationWindow(QWidget *parent = nullptr);
    ~AboutApplicationWindow() noexcept override;

    AboutApplicationWindow(const AboutApplicationWindow&) = delete;
    AboutApplicationWindow& operator= (const AboutApplicationWindow&) = delete;

    AboutApplicationWindow(AboutApplicationWindow&&) = delete;
    AboutApplicationWindow& operator= (AboutApplicationWindow&&) = delete;

private:
    Ui::AboutApplicationWindow ui;
};

#endif