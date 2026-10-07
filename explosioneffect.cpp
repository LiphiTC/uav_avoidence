#include "explosioneffect.h"
#include "videotexture.h"

#include <QMediaPlayer>
#include <QVideoFrame>
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DExtras/QTextureMaterial>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QNoDepthMask>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QTexture>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QVideoSink>
#else
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <functional>

namespace {
class VideoSurface : public QAbstractVideoSurface
{
public:
    explicit VideoSurface(QObject *parent) : QAbstractVideoSurface(parent) {}
    std::function<void(const QImage &)> onFrame;

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
        QAbstractVideoBuffer::HandleType type) const override
    {
        if (type != QAbstractVideoBuffer::NoHandle)
            return {};
        return {QVideoFrame::Format_ARGB32, QVideoFrame::Format_RGB32, QVideoFrame::Format_RGB24};
    }

    bool present(const QVideoFrame &input) override
    {
        QVideoFrame frame(input);
        if (!frame.map(QAbstractVideoBuffer::ReadOnly))
            return false;
        const QImage::Format format = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
        QImage image(frame.bits(), frame.width(), frame.height(), frame.bytesPerLine(), format);
        image = image.copy();
        frame.unmap();
        if (surfaceFormat().scanLineDirection() == QVideoSurfaceFormat::BottomToTop)
            image = image.mirrored();
        // GStreamer может передавать кадры из рабочего потока.
        QMetaObject::invokeMethod(this, [this, image] { onFrame(image); }, Qt::QueuedConnection);
        return !image.isNull();
    }
};
}
#endif

ExplosionEffect::ExplosionEffect(Qt3DCore::QEntity *root, Qt3DRender::QCamera *camera, QObject *parent)
    : QObject(parent), m_entity(new Qt3DCore::QEntity(root)),
      m_transform(new Qt3DCore::QTransform(m_entity)), m_camera(camera),
      m_texture(nullptr), m_player(new QMediaPlayer(this))
{
    m_entity->setObjectName("explosionPlane");
    m_entity->setEnabled(false);
    Qt3DExtras::QPlaneMesh *mesh = new Qt3DExtras::QPlaneMesh(m_entity);
    mesh->setWidth(160);
    mesh->setHeight(90);
    Qt3DRender::QTexture2D *texture = new Qt3DRender::QTexture2D(m_entity);
    texture->setFormat(Qt3DRender::QAbstractTexture::RGBA8_UNorm);
    texture->setGenerateMipMaps(false);
    texture->setMinificationFilter(Qt3DRender::QAbstractTexture::Linear);
    texture->setMagnificationFilter(Qt3DRender::QAbstractTexture::Linear);
    m_texture = new VideoTexture(texture);
    texture->addTextureImage(m_texture);
    Qt3DExtras::QTextureMaterial *material = new Qt3DExtras::QTextureMaterial(m_entity);
    material->setTexture(texture);
    material->setAlphaBlendingEnabled(true);
    for (Qt3DRender::QTechnique *technique : material->effect()->techniques()) {
        for (Qt3DRender::QRenderPass *pass : technique->renderPasses())
            pass->addRenderState(new Qt3DRender::QNoDepthMask(pass));
    }
    m_entity->addComponent(mesh);
    m_entity->addComponent(m_transform);
    m_entity->addComponent(material);
    connect(camera, &Qt3DRender::QCamera::viewMatrixChanged, this, &ExplosionEffect::faceCamera);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia)
            stop();
    });

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QVideoSink *sink = new QVideoSink(m_player);
    m_player->setVideoSink(sink);
    connect(sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame &frame) {
        if (m_playing && frame.isValid())
            showFrame(frame.toImage());
    });
    connect(m_player, &QMediaPlayer::errorOccurred, this, [this] {
        stop();
        emit errorOccurred(tr("Не удалось воспроизвести explosion.mp4: %1").arg(m_player->errorString()));
    });
    m_player->setSource(QUrl("qrc:/assets/explosion.mp4"));
#else
    VideoSurface *surface = new VideoSurface(m_player);
    surface->onFrame = [this](const QImage &frame) { showFrame(frame); };
    m_player->setVideoOutput(surface);
    m_player->setMuted(true);
    connect(m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this, [this] {
        stop();
        emit errorOccurred(tr("Не удалось воспроизвести explosion.mp4: %1").arg(m_player->errorString()));
    });
    m_player->setMedia(QUrl("qrc:/assets/explosion.mp4"));
#endif
}

ExplosionEffect::~ExplosionEffect()
{
    stop();
    delete m_player;
    delete m_entity;
}

void ExplosionEffect::play(const QVector3D &target)
{
    stop();
    m_target = target;
    m_playing = true;
    faceCamera();
    m_player->setPosition(0);
    m_player->play();
}

void ExplosionEffect::stop()
{
    m_playing = false;
    m_player->stop();
    m_entity->setEnabled(false);
    m_texture->clear();
}

void ExplosionEffect::showFrame(const QImage &frame)
{
    if (!m_playing || frame.isNull())
        return;
    m_texture->setFrame(frame);
    m_entity->setEnabled(true);
    emit frameReady();
}

void ExplosionEffect::faceCamera()
{
    if (!m_playing)
        return;
    const QVector3D towardCamera = (m_camera->position() - m_target).normalized();
    // Сдвиг к камере размещает видео перед кубом цели и БПЛА.
    m_transform->setTranslation(m_target + towardCamera * 16);
    m_transform->setRotation(QQuaternion::fromDirection(towardCamera, m_camera->upVector())
        * QQuaternion::fromAxisAndAngle({1, 0, 0}, 90));
}
