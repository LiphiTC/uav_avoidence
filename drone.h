#ifndef DRONE_H
#define DRONE_H

#include <cstddef>
#include <vector>
#include "point3d.h"

class Drone
{
public:
    enum class State { Ready, Running, Paused, Arrived };

    void reset(const Point3D &position);
    void setRoute(const std::vector<Point3D> &route);
    void setSpeed(double speed);
    bool start();
    void pause();
    void step(double dt);

    Point3D position() const { return m_position; }
    State state() const { return m_state; }
    double speed() const { return m_speed; }
    const std::vector<Point3D> &route() const { return m_route; }
    static constexpr double radius = 8;

private:
    Point3D m_position;
    State m_state = State::Ready;
    double m_speed = 80;
    std::vector<Point3D> m_route;
    std::size_t m_index = 0;
};

#endif
