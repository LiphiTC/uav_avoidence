#ifndef EXPLOSIONEFFECT_H
#define EXPLOSIONEFFECT_H

#include <QObject>
#include <QVector3D>

namespace Qt3DCore { class QEntity; class QTransform; }
namespace Qt3DRender { class QCamera; }
class QMediaPlayer;
class QImage;
class VideoTexture;

class ExplosionEffect : public QObject
{
    Q_OBJECT

public:
    ExplosionEffect(Qt3DCore::QEntity *root, Qt3DRender::QCamera *camera, QObject *parent = nullptr);
    ~ExplosionEffect() override;
    void play(const QVector3D &target);
    void stop();
    bool isPlaying() const { return m_playing; }

signals:
    void frameReady();
    void errorOccurred(const QString &message);

private:
    void showFrame(const QImage &frame);
    void faceCamera();

    Qt3DCore::QEntity *m_entity;
    Qt3DCore::QTransform *m_transform;
    Qt3DRender::QCamera *m_camera;
    VideoTexture *m_texture;
    QMediaPlayer *m_player;
    QVector3D m_target;
    bool m_playing = false;
};

#endif
