#include "dronevisual.h"

#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DExtras/QTextureMaterial>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QTexture>
#include <QImage>

DroneVisual::DroneVisual(Qt3DRender::QCamera *camera, Qt3DCore::QEntity *parent)
    : QEntity(parent), m_transform(new Qt3DCore::QTransform(this)), m_camera(camera)
{
    setObjectName("droneVisual");
    const QImage image(":/assets/drone.jpg");
    Qt3DExtras::QPlaneMesh *mesh = new Qt3DExtras::QPlaneMesh(this);
    mesh->setWidth(42);
    mesh->setHeight(image.isNull() ? 30 : 42.0f * image.height() / image.width());
    Qt3DRender::QTextureLoader *texture = new Qt3DRender::QTextureLoader(this);
    texture->setSource(QUrl("qrc:/assets/drone.jpg"));
    Qt3DExtras::QTextureMaterial *material = new Qt3DExtras::QTextureMaterial(this);
    material->setTexture(texture);
    addComponent(mesh);
    addComponent(material);
    addComponent(m_transform);
    connect(camera, &Qt3DRender::QCamera::viewMatrixChanged, this, &DroneVisual::faceCamera);
    faceCamera();
}

void DroneVisual::setPosition(const QVector3D &position)
{
    m_transform->setTranslation(position);
    faceCamera();
}

void DroneVisual::faceCamera()
{
    const QVector3D towardCamera = (m_camera->position() - m_transform->translation()).normalized();
    m_transform->setRotation(QQuaternion::fromDirection(towardCamera, m_camera->upVector())
        * QQuaternion::fromAxisAndAngle({1, 0, 0}, 90));
}
