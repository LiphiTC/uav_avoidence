#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "drone.h"
#include "zone.h"

class Environment
{
public:
    enum class Result { Direct, Detour, InvalidInput, StartInZone, GoalInZone };

    Environment();
    void reset();
    Result planRoute(const Point3D &start, const Point3D &goal, double radius);
    void clearRoute();
    void setSpeed(double speed) { m_drone.setSpeed(speed); }
    bool startFlight() { return m_drone.start(); }
    void pauseFlight() { m_drone.pause(); }
    void step(double dt) { m_drone.step(dt); }

    const Zone &zone() const { return m_zone; }
    const Drone &drone() const { return m_drone; }
    Point3D start() const { return m_start; }
    Point3D goal() const { return m_goal; }
    double routeLength() const;
    static bool validPosition(const Point3D &position);
    static constexpr double clearance = 2;
    static constexpr double minRadius = 1;
    static constexpr double maxRadius = 180;

private:
    std::vector<Point3D> buildDetour() const;

    Zone m_zone;
    Drone m_drone;
    Point3D m_start;
    Point3D m_goal;
};

#endif
