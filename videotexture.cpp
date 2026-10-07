#include "videotexture.h"

#include <QPainter>
#include <algorithm>

VideoTexture::VideoTexture(Qt3DCore::QNode *parent)
    : QPaintedTextureImage(parent)
{
    setSize({640, 360});
}

QImage VideoTexture::removeGreenScreen(const QImage &frame)
{
    QImage image = frame.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        QRgb *pixels = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = pixels[x];
            const int red = qRed(pixel);
            const int green = qGreen(pixel);
            const int blue = qBlue(pixel);
            const int excess = green - std::max(red, blue);
            // Мягкий переход убирает зелёный фон и ореол на краях огня.
            const int opacity = 255 - std::clamp((excess - 15) * 255 / 65, 0, 255);
            const int correctedGreen = excess > 15 ? std::min(green, std::max(red, blue)) : green;
            pixels[x] = qRgba(red, correctedGreen, blue, qAlpha(pixel) * opacity / 255);
        }
    }
    return image;
}

void VideoTexture::setFrame(const QImage &frame)
{
    m_frame = removeGreenScreen(frame.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    update();
}

void VideoTexture::clear()
{
    m_frame = {};
    update();
}

void VideoTexture::paint(QPainter *painter)
{
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    painter->fillRect(QRect(QPoint(), size()), Qt::transparent);
    // QImage начинается сверху, координаты текстуры OpenGL — снизу.
    painter->setViewport(0, height(), width(), -height());
    painter->translate(width(), height());
    painter->rotate(180);
    if (!m_frame.isNull())
        painter->drawImage(0, 0, m_frame);
}
