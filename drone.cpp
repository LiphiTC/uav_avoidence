#include "drone.h"

#include <algorithm>

void Drone::reset(const Point3D &position)
{
    m_position = position;
    m_state = State::Ready;
    m_route.clear();
    m_index = 0;
}

void Drone::setRoute(const std::vector<Point3D> &route)
{
    reset(route.empty() ? m_position : route.front());
    m_route = route;
    m_index = m_route.empty() ? 0 : 1;
}

void Drone::setSpeed(double speed)
{
    if (std::isfinite(speed))
        m_speed = std::clamp(speed, 10.0, 200.0);
}

bool Drone::start()
{
    if (m_route.empty() || m_state == State::Arrived)
        return false;
    m_state = State::Running;
    return true;
}

void Drone::pause()
{
    if (m_state == State::Running)
        m_state = State::Paused;
}

void Drone::step(double dt)
{
    if (m_state != State::Running || !std::isfinite(dt) || dt <= 0)
        return;
    double remaining = m_speed * dt;
    while (m_index < m_route.size()) {
        const Point3D target = m_route.at(m_index);
        const double segment = distance(m_position, target);
        if (segment > remaining) {
            m_position = m_position + (target - m_position) * (remaining / segment);
            return;
        }
        m_position = target;
        remaining -= segment;
        ++m_index;
    }
    m_state = State::Arrived;
}
