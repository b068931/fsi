#ifndef ABOUT_APPLICATION_WINDOW_H
#define ABOUT_APPLICATION_WINDOW_H

#include <QWidget>
#include <QEvent>

#include "ui_about_application_window.h"
#include "SVGImage.h"

namespace Windows {
    /// <summary>
    /// Represents a window displaying information about the application.
    /// This includes short overview and attributions.
    /// </summary>
    class AboutApplicationWindow : public QWidget {
        Q_OBJECT

    public:
        explicit AboutApplicationWindow(QWidget* parent = nullptr);
        ~AboutApplicationWindow() noexcept override;

        AboutApplicationWindow(const AboutApplicationWindow&) = delete;
        AboutApplicationWindow& operator= (const AboutApplicationWindow&) = delete;

        AboutApplicationWindow(AboutApplicationWindow&&) = delete;
        AboutApplicationWindow& operator= (AboutApplicationWindow&&) = delete;

    public slots:
        void onRetranslateUI() noexcept;

    protected:
        bool event(QEvent* event) override;

    private:
        Ui::AboutApplicationWindow ui;
        CustomWidgets::SVGImage* aboutTopImage;

        void setupTopImage();
        void allowExternalLinks() noexcept;
    };
}

#endif