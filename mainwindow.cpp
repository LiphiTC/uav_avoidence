#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scene3d.h"

#include <QDoubleValidator>
#include <QLocale>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_scene(nullptr), m_timer(new QTimer(this))
{
    ui->setupUi(this);
    m_scene = new Scene3D(ui->sceneHost);
    ui->sceneLayout->addWidget(m_scene);
    m_timer->setInterval(20);
    m_timer->setTimerType(Qt::PreciseTimer);
    ui->titleLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
    ui->labelInputError->setStyleSheet("color: #d34c43;");
    connect(m_scene, &Scene3D::effectError, ui->labelInputError, &QLabel::setText);

    const std::vector<QLineEdit *> fields{ui->lineEditStartX, ui->lineEditStartY,
        ui->lineEditStartZ, ui->lineEditGoalX, ui->lineEditGoalY,
        ui->lineEditGoalZ, ui->lineEditRadius};
    for (QLineEdit *field : fields) {
        const bool radius = field == ui->lineEditRadius;
        const bool y = field == ui->lineEditStartY || field == ui->lineEditGoalY;
        QDoubleValidator *validator = new QDoubleValidator(
            radius ? Environment::minRadius : Drone::radius,
            radius ? Environment::maxRadius : (y ? 392 : 592), 2, field);
        validator->setNotation(QDoubleValidator::StandardNotation);
        QLocale locale = QLocale::c();
        locale.setNumberOptions(QLocale::RejectGroupSeparator);
        validator->setLocale(locale);
        field->setValidator(validator);
        field->setToolTip(tr("Дробная часть — через точку, до двух знаков."));
        connect(field, &QLineEdit::returnPressed, this, &MainWindow::planRoute);
        connect(field, &QLineEdit::textEdited, this, &MainWindow::invalidateRoute);
        connect(field, &QLineEdit::inputRejected, this, [this] {
            ui->labelInputError->setText(tr("Введите X и Z: 8–592, Y: 8–392; радиус: 1–180."));
            m_scene->playCriticalSound();
        }, Qt::QueuedConnection);
    }

    connect(ui->btnRoute, &QPushButton::clicked, this, &MainWindow::planRoute);
    connect(ui->btnStartPause, &QPushButton::clicked, this, &MainWindow::toggleFlight);
    connect(ui->btnReset, &QPushButton::clicked, this, &MainWindow::resetSimulation);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::moveDrone);
    connect(ui->sliderSpeed, &QSlider::valueChanged, this, [this](int speed) {
        m_environment.setSpeed(speed);
        updateDisplay();
    });
    connect(ui->btnZoomIn, &QPushButton::clicked, this, [this] { m_scene->zoom(1 / 1.1f); });
    connect(ui->btnZoomOut, &QPushButton::clicked, this, [this] { m_scene->zoom(1.1f); });
    connect(ui->btnCameraReset, &QPushButton::clicked, m_scene, &Scene3D::resetCamera);
    resetSimulation();
}

MainWindow::~MainWindow()
{
    m_timer->stop();
    delete ui;
}

void MainWindow::planRoute()
{
    m_timer->stop();
    m_environment.clearRoute();
    const std::vector<QLineEdit *> fields{ui->lineEditStartX, ui->lineEditStartY,
        ui->lineEditStartZ, ui->lineEditGoalX, ui->lineEditGoalY,
        ui->lineEditGoalZ, ui->lineEditRadius};
    for (QLineEdit *field : fields) {
        if (!field->hasAcceptableInput()) {
            ui->labelInputError->setText(tr("Введите X и Z: 8–592, Y: 8–392; радиус: 1–180."));
            field->setFocus();
            m_scene->showEnvironment(m_environment);
            updateDisplay();
            m_scene->playCriticalSound();
            return;
        }
    }
    const Point3D start{ui->lineEditStartX->text().toDouble(),
                        ui->lineEditStartY->text().toDouble(),
                        ui->lineEditStartZ->text().toDouble()};
    const Point3D goal{ui->lineEditGoalX->text().toDouble(),
                       ui->lineEditGoalY->text().toDouble(),
                       ui->lineEditGoalZ->text().toDouble()};
    const Environment::Result result = m_environment.planRoute(
        start, goal, ui->lineEditRadius->text().toDouble());
    ui->labelInputError->clear();
    switch (result) {
    case Environment::Result::Direct:
        ui->labelRoute->setText(tr("Прямой маршрут: зона не пересекается."));
        break;
    case Environment::Result::Detour:
        ui->labelRoute->setText(tr("Обход сферы: касательные и дуга в 3D."));
        break;
    case Environment::Result::StartInZone:
        ui->labelInputError->setText(tr("Старт внутри зоны или защитного зазора (10 ед.)."));
        break;
    case Environment::Result::GoalInZone:
        ui->labelInputError->setText(tr("Цель внутри зоны или защитного зазора (10 ед.)."));
        break;
    case Environment::Result::InvalidInput:
        ui->labelInputError->setText(tr("Проверьте координаты и радиус зоны."));
        break;
    }
    m_scene->showEnvironment(m_environment);
    updateDisplay();
    if (result == Environment::Result::InvalidInput || result == Environment::Result::StartInZone
        || result == Environment::Result::GoalInZone)
        m_scene->playCriticalSound();
}

void MainWindow::invalidateRoute()
{
    m_timer->stop();
    m_environment.clearRoute();
    ui->labelInputError->clear();
    m_scene->showEnvironment(m_environment);
    updateDisplay();
}

void MainWindow::toggleFlight()
{
    if (m_timer->isActive()) {
        m_timer->stop();
        m_environment.pauseFlight();
    } else {
        if (m_environment.drone().state() == Drone::State::Arrived) {
            planRoute();
            if (m_environment.drone().route().empty())
                return;
        }
        if (!m_environment.startFlight())
            return;
        m_elapsed.start();
        m_timer->start();
    }
    updateDisplay();
}

void MainWindow::moveDrone()
{
    m_environment.step(m_elapsed.restart() / 1000.0);
    if (m_environment.drone().state() != Drone::State::Running)
        m_timer->stop();
    updateDisplay();
}

void MainWindow::resetSimulation()
{
    m_timer->stop();
    m_environment.reset();
    ui->lineEditStartX->setText("50");
    ui->lineEditStartY->setText("350");
    ui->lineEditStartZ->setText("100");
    ui->lineEditGoalX->setText("550");
    ui->lineEditGoalY->setText("50");
    ui->lineEditGoalZ->setText("280");
    ui->lineEditRadius->setText("120");
    ui->sliderSpeed->setValue(80);
    ui->labelInputError->clear();
    m_scene->resetCamera();
    m_scene->showEnvironment(m_environment);
    updateDisplay();
}

void MainWindow::updateDisplay()
{
    const Drone &drone = m_environment.drone();
    const bool hasRoute = !drone.route().empty();
    const bool running = drone.state() == Drone::State::Running;
    ui->btnStartPause->setEnabled(hasRoute);
    ui->btnStartPause->setText(running ? tr("Пауза")
        : (drone.state() == Drone::State::Arrived ? tr("Повторить полёт") : tr("Старт")));
    ui->labelLength->setText(hasRoute
        ? tr("Длина пути: %1 ед.").arg(m_environment.routeLength(), 0, 'f', 1)
        : tr("Длина пути: —"));
    if (!hasRoute)
        ui->labelRoute->setText(tr("Нажмите «Проложить маршрут»."));
    QString state;
    switch (drone.state()) {
    case Drone::State::Ready: state = tr("Готов к полёту"); break;
    case Drone::State::Running: state = tr("Полёт"); break;
    case Drone::State::Paused: state = tr("Пауза"); break;
    case Drone::State::Arrived: state = tr("Цель достигнута"); break;
    }
    const Point3D p = drone.position();
    ui->labelStatus->setText(tr("%1\nX = %2   Y = %3   Z = %4")
        .arg(state).arg(p.x, 0, 'f', 1).arg(p.y, 0, 'f', 1).arg(p.z, 0, 'f', 1));
    ui->labelSpeed->setText(tr("Скорость: %1 ед./с").arg(drone.speed(), 0, 'f', 0));
    m_scene->updateDrone(drone);
}
