#ifndef VIDEOTEXTURE_H
#define VIDEOTEXTURE_H

#include <QImage>
#include <Qt3DRender/QPaintedTextureImage>

class VideoTexture : public Qt3DRender::QPaintedTextureImage
{
public:
    explicit VideoTexture(Qt3DCore::QNode *parent = nullptr);
    void setFrame(const QImage &frame);
    void clear();
    static QImage removeGreenScreen(const QImage &frame);

protected:
    void paint(QPainter *painter) override;

private:
    QImage m_frame;
};

#endif
