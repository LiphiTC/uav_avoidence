#ifndef SCENE3D_H
#define SCENE3D_H

#include <QWidget>
#include <QVector3D>
#include "environment.h"

namespace Qt3DCore { class QEntity; class QTransform; }
namespace Qt3DExtras { class Qt3DWindow; class QPhongMaterial; }
class ExplosionEffect;
class DroneVisual;
class FlightAudio;

class Scene3D : public QWidget
{
    Q_OBJECT

public:
    explicit Scene3D(QWidget *parent = nullptr);
    ~Scene3D() override;
    void showEnvironment(const Environment &environment);
    void updateDrone(const Drone &drone);
    void zoom(float factor);
    void resetCamera();
    void playCriticalSound();

signals:
    void effectError(const QString &message);

private:
    static QVector3D toScene(const Point3D &position);
    Qt3DCore::QEntity *addBox(const Point3D &center, const Point3D &size,
                            const QColor &color, Qt3DCore::QEntity *parent);
    void addLine(const Point3D &from, const Point3D &to, const QColor &color,
                 Qt3DCore::QEntity *parent, float width = 1.5f);
    void addZone(const Zone &zone, Qt3DCore::QEntity *parent);
    void updateAspectRatio();

    Qt3DExtras::Qt3DWindow *m_view;
    QWidget *m_container;
    Qt3DCore::QEntity *m_root;
    Qt3DCore::QEntity *m_environment = nullptr;
    DroneVisual *m_droneVisual;
    FlightAudio *m_audio;
    ExplosionEffect *m_explosion;
    Drone::State m_previousState = Drone::State::Ready;
};

#endif
