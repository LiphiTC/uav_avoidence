#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QElapsedTimer>
#include "environment.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
class QTimer;
class Scene3D;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void planRoute();
    void toggleFlight();
    void moveDrone();
    void resetSimulation();
    void invalidateRoute();
    void updateDisplay();

    Ui::MainWindow *ui;
    Scene3D *m_scene;
    QTimer *m_timer;
    QElapsedTimer m_elapsed;
    Environment m_environment;
};

#endif
