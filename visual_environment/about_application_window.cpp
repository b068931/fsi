#include "about_application_window.h"

namespace Windows {
    AboutApplicationWindow::AboutApplicationWindow(QWidget* parent)
        : QWidget(parent), aboutTopImage(nullptr)
    {
        constexpr int fixedWidth = 550;
        constexpr int fixedHeight = 420;

        this->ui.setupUi(this);
        this->setupTopImage();
        this->allowExternalLinks();

        this->setWindowFlag(Qt::Window);
        this->setFixedSize(fixedWidth, fixedHeight);
    }

    AboutApplicationWindow::~AboutApplicationWindow() noexcept = default;

    void AboutApplicationWindow::setupTopImage() {
        constexpr int imageWidth = 100;
        constexpr int imageHeight = 100;
        constexpr int imageStretch = 0;
        constexpr int insertTop = 0;

        this->aboutTopImage = new CustomWidgets::SVGImage(
            ":/icons/about_application_image.svg",
            QSize(imageWidth, imageHeight),
            this
        );

        this->ui.verticalLayout->insertWidget(
            insertTop,
            this->aboutTopImage,
            imageStretch,
            Qt::AlignVCenter | Qt::AlignHCenter
        );
    }

    void AboutApplicationWindow::allowExternalLinks() noexcept {
        this->ui.aboutApplication->setOpenExternalLinks(true);
        this->ui.builtWith->setOpenExternalLinks(true);
        this->ui.openMojiAttribution->setOpenExternalLinks(true);
    }
}
