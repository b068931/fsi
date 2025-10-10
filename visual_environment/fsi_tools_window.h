#ifndef FSI_TOOLS_WINDOW_H
#define FSI_TOOLS_WINDOW_H

#include <QWidget>
#include "ui_fsi_tools_window.h"

namespace Windows {
    class FsiToolsWindow : public QWidget
    {
        Q_OBJECT

    public:
        explicit FsiToolsWindow(QWidget* parent = nullptr);
        ~FsiToolsWindow() noexcept override;

        FsiToolsWindow(const FsiToolsWindow&) = delete;
        FsiToolsWindow& operator= (const FsiToolsWindow&) = delete;

        FsiToolsWindow(FsiToolsWindow&&) = delete;
        FsiToolsWindow& operator= (FsiToolsWindow&&) = delete;

    private:
        Ui::FSIToolsForm ui;
    };
}

#endif