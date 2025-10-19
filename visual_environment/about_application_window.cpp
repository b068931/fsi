#include "about_application_window.h"

AboutApplicationWindow::AboutApplicationWindow(QWidget *parent)
    : QWidget(parent)
{
    constexpr int fixedWidth = 550;
    constexpr int fixedHeight = 420;

    this->ui.setupUi(this);
    this->setFixedSize(fixedWidth, fixedHeight);
}

AboutApplicationWindow::~AboutApplicationWindow() noexcept = default;