#ifndef SVG_IMAGE_WIDGET_H
#define SVG_IMAGE_WIDGET_H

#include <QSize>
#include <QWidget>
#include <QString>
#include <QPaintEvent>
#include <QSvgRenderer>

namespace CustomWidgets {
    class SVGImage : public QWidget {
        Q_OBJECT

    public:
        explicit SVGImage(const QString& imagePath, const QSize& size, QWidget* parent = nullptr);
        ~SVGImage() noexcept override;

        SVGImage(const SVGImage&) = delete;
        SVGImage& operator=(const SVGImage&) = delete;

        SVGImage(SVGImage&&) = delete;
        SVGImage& operator=(SVGImage&&) = delete;

        void setImageSize(const QSize& size);

    protected:
        virtual void paintEvent(QPaintEvent* event) override;
        virtual QSize sizeHint() const override;

    private:
        QSvgRenderer svgRenderer;
        QSize imageSize;
    };
}

#endif