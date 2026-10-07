#include "scene3d.h"
#include "explosioneffect.h"
#include "dronevisual.h"
#include "flightaudio.h"

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QPhongAlphaMaterial>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QDirectionalLight>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

QVector3D Scene3D::toScene(const Point3D &p)
{
    return {float(p.x - 300), float(p.z), float(200 - p.y)};
}

Scene3D::Scene3D(QWidget *parent)
    : QWidget(parent), m_view(new Qt3DExtras::Qt3DWindow),
      m_container(QWidget::createWindowContainer(m_view, this)),
      m_root(new Qt3DCore::QEntity)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_container);
    m_container->setMinimumSize(480, 360);
    m_container->setFocusPolicy(Qt::StrongFocus);
    m_view->defaultFrameGraph()->setClearColor(QColor(23, 32, 46));

    addBox({300, 200, -3}, {600, 400, 6}, QColor(48, 62, 75), m_root);
    for (int x = 0; x <= 600; x += 50)
        addLine({double(x), 0, 0.5}, {double(x), 400, 0.5}, QColor(80, 95, 110), m_root, 0.7f);
    for (int y = 0; y <= 400; y += 50)
        addLine({0, double(y), 0.5}, {600, double(y), 0.5}, QColor(80, 95, 110), m_root, 0.7f);
    addLine({0, 0, 2}, {600, 0, 2}, QColor(235, 90, 90), m_root, 2);
    addLine({0, 0, 2}, {0, 400, 2}, QColor(90, 220, 145), m_root, 2);
    addLine({0, 0, 2}, {0, 0, 600}, QColor(80, 160, 255), m_root, 2);

    m_droneVisual = new DroneVisual(m_view->camera(), m_root);
    m_audio = new FlightAudio(this);
    connect(m_audio, &FlightAudio::errorOccurred, this, &Scene3D::effectError);

    m_explosion = new ExplosionEffect(m_root, m_view->camera(), this);
    connect(m_explosion, &ExplosionEffect::errorOccurred, this, &Scene3D::effectError);

    Qt3DCore::QEntity *lightEntity = new Qt3DCore::QEntity(m_root);
    Qt3DRender::QDirectionalLight *light = new Qt3DRender::QDirectionalLight(lightEntity);
    light->setWorldDirection({-0.5f, -1.0f, -0.4f});
    light->setIntensity(1.1f);
    lightEntity->addComponent(light);

    Qt3DExtras::QOrbitCameraController *controller = new Qt3DExtras::QOrbitCameraController(m_root);
    controller->setCamera(m_view->camera());
    controller->setLinearSpeed(400);
    controller->setLookSpeed(120);
    connect(m_view, &QWindow::widthChanged, this, [this] { updateAspectRatio(); });
    connect(m_view, &QWindow::heightChanged, this, [this] { updateAspectRatio(); });
    resetCamera();
    m_view->setRootEntity(m_root);
}

Scene3D::~Scene3D()
{
    delete m_audio;
    delete m_explosion;
    m_view->setRootEntity(nullptr);
    delete m_container;
    delete m_root;
}

Qt3DCore::QEntity *Scene3D::addBox(const Point3D &center, const Point3D &size,
                                 const QColor &color, Qt3DCore::QEntity *parent)
{
    Qt3DCore::QEntity *entity = new Qt3DCore::QEntity(parent);
    Qt3DExtras::QCuboidMesh *mesh = new Qt3DExtras::QCuboidMesh(entity);
    mesh->setXExtent(size.x);
    mesh->setYExtent(size.z);
    mesh->setZExtent(size.y);
    Qt3DCore::QTransform *transform = new Qt3DCore::QTransform(entity);
    transform->setTranslation(toScene(center));
    Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial(entity);
    material->setDiffuse(color);
    material->setAmbient(color.darker(180));
    entity->addComponent(mesh);
    entity->addComponent(transform);
    entity->addComponent(material);
    return entity;
}

void Scene3D::addLine(const Point3D &from, const Point3D &to, const QColor &color,
                      Qt3DCore::QEntity *parent, float width)
{
    const QVector3D direction = toScene(to) - toScene(from);
    if (direction.lengthSquared() < 0.0001f)
        return;
    Qt3DCore::QEntity *entity = new Qt3DCore::QEntity(parent);
    Qt3DExtras::QCylinderMesh *mesh = new Qt3DExtras::QCylinderMesh(entity);
    mesh->setRadius(width);
    mesh->setLength(direction.length());
    mesh->setRings(2);
    mesh->setSlices(8);
    Qt3DCore::QTransform *transform = new Qt3DCore::QTransform(entity);
    const Point3D center{(from.x + to.x) / 2, (from.y + to.y) / 2, (from.z + to.z) / 2};
    transform->setTranslation(toScene(center));
    transform->setRotation(QQuaternion::rotationTo({0, 1, 0}, direction.normalized()));
    Qt3DExtras::QPhongMaterial *material = new Qt3DExtras::QPhongMaterial(entity);
    material->setDiffuse(color);
    material->setAmbient(color.darker(130));
    entity->addComponent(mesh);
    entity->addComponent(transform);
    entity->addComponent(material);
}

void Scene3D::addZone(const Zone &zone, Qt3DCore::QEntity *parent)
{
    Qt3DCore::QEntity *sphere = new Qt3DCore::QEntity(parent);
    Qt3DExtras::QSphereMesh *mesh = new Qt3DExtras::QSphereMesh(sphere);
    mesh->setRadius(zone.radius());
    mesh->setRings(32);
    mesh->setSlices(48);
    Qt3DCore::QTransform *transform = new Qt3DCore::QTransform(sphere);
    transform->setTranslation(toScene(zone.center()));
    Qt3DExtras::QPhongAlphaMaterial *material = new Qt3DExtras::QPhongAlphaMaterial(sphere);
    material->setDiffuse(QColor(245, 196, 60));
    material->setAmbient(QColor(150, 115, 35));
    material->setAlpha(0.18f);
    sphere->addComponent(mesh);
    sphere->addComponent(transform);
    sphere->addComponent(material);

    const double pi = 3.14159265358979323846;
    for (int plane = 0; plane < 3; ++plane) {
        Point3D previous;
        for (int i = 0; i <= 64; ++i) {
            const double angle = 2 * pi * i / 64;
            const double a = zone.radius() * std::cos(angle);
            const double b = zone.radius() * std::sin(angle);
            const Point3D offset = plane == 0 ? Point3D{a, b, 0}
                : (plane == 1 ? Point3D{a, 0, b} : Point3D{0, a, b});
            const Point3D point = zone.center() + offset;
            if (i > 0)
                addLine(previous, point, QColor(245, 196, 60), parent, 0.8f);
            previous = point;
        }
    }
}

void Scene3D::showEnvironment(const Environment &environment)
{
    m_audio->reset();
    m_explosion->stop();
    m_previousState = Drone::State::Ready;
    delete m_environment;
    m_environment = new Qt3DCore::QEntity(m_root);
    addZone(environment.zone(), m_environment);
    addBox(environment.start(), {14, 14, 14}, QColor(70, 220, 120), m_environment);
    addBox(environment.goal(), {14, 14, 14}, QColor(235, 65, 65), m_environment);
    const std::vector<Point3D> &points = environment.drone().route();
    for (std::size_t i = 1; i < points.size(); ++i)
        addLine(points.at(i - 1), points.at(i), QColor(60, 155, 255), m_environment, 2);
    updateDrone(environment.drone());
}

void Scene3D::updateDrone(const Drone &drone)
{
    if (drone.state() == Drone::State::Arrived && m_previousState != Drone::State::Arrived)
        m_explosion->play(toScene(drone.position()));
    else if (drone.state() != Drone::State::Arrived && m_previousState == Drone::State::Arrived)
        m_explosion->stop();
    m_previousState = drone.state();
    m_droneVisual->setPosition(toScene(drone.position()));
    m_audio->updateState(drone.state());
}

void Scene3D::updateAspectRatio()
{
    m_view->camera()->lens()->setPerspectiveProjection(
        45, float(std::max(1, m_view->width())) / std::max(1, m_view->height()), 1, 6000);
}

void Scene3D::playCriticalSound()
{
    m_audio->playCritical();
}

void Scene3D::resetCamera()
{
    m_view->camera()->setViewCenter({0, 180, 0});
    m_view->camera()->setPosition({700, 700, 900});
    m_view->camera()->setUpVector({0, 1, 0});
    updateAspectRatio();
}

void Scene3D::zoom(float factor)
{
    Qt3DRender::QCamera *camera = m_view->camera();
    const QVector3D offset = camera->position() - camera->viewCenter();
    const float distance = std::clamp(offset.length() * factor, 150.0f, 3000.0f);
    camera->setPosition(camera->viewCenter() + offset.normalized() * distance);
}
