#include <QPainter>
#include "SVGImage.h"

namespace CustomWidgets {
    SVGImage::SVGImage(const QString& imagePath, const QSize& size, QWidget* parent)
        : QWidget(parent), imageSize(size) {
        this->svgRenderer.load(imagePath);
        if (!this->svgRenderer.isValid()) {
            qWarning() << "Failed to load SVG image from path:" << imagePath;
        }
    }

    SVGImage::~SVGImage() noexcept = default;

    void SVGImage::setImageSize(const QSize& size) {
        this->imageSize = size;
        this->updateGeometry(); // Notify layout system of size hint change
        this->update();         // Trigger a repaint
    }

    QSize SVGImage::sizeHint() const {
        return this->imageSize;
    }

    void SVGImage::paintEvent(QPaintEvent* event) {
        QPainter painter(this);
        painter.setClipRegion(event->region());
        painter.setRenderHint(QPainter::Antialiasing, true);

        if (this->svgRenderer.isValid()) {
            this->svgRenderer.render(&painter, event->rect());
        }
        else {
            qInfo() << "SVG renderer is not valid. Won't paint SVG image.";
        }
    }
}
