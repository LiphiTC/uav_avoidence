#include "environment.h"

#include <algorithm>

namespace {
constexpr double pi = 3.14159265358979323846;

double pathLength(const std::vector<Point3D> &points)
{
    double total = 0;
    for (std::size_t i = 1; i < points.size(); ++i)
        total += distance(points.at(i - 1), points.at(i));
    return total;
}
}

Environment::Environment()
{
    reset();
}

void Environment::reset()
{
    m_zone = Zone();
    m_start = {50, 350, 100};
    m_goal = {550, 50, 280};
    m_drone.reset(m_start);
    m_drone.setSpeed(80);
}

void Environment::clearRoute()
{
    m_drone.reset(m_start);
}

bool Environment::validPosition(const Point3D &p)
{
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z)
        && p.x >= Drone::radius && p.x <= 600 - Drone::radius
        && p.y >= Drone::radius && p.y <= 400 - Drone::radius
        && p.z >= Drone::radius && p.z <= 600 - Drone::radius;
}

Environment::Result Environment::planRoute(const Point3D &start, const Point3D &goal,
                                            double radius)
{
    clearRoute();
    if (!validPosition(start) || !validPosition(goal) || !std::isfinite(radius)
        || radius < minRadius || radius > maxRadius)
        return Result::InvalidInput;

    m_start = start;
    m_goal = goal;
    m_zone = Zone({300, 200, 200}, radius);
    m_drone.reset(m_start);
    const double margin = Drone::radius + clearance;
    if (m_zone.contains(start, margin))
        return Result::StartInZone;
    if (m_zone.contains(goal, margin))
        return Result::GoalInZone;

    if (!m_zone.intersects(start, goal, margin)) {
        m_drone.setRoute({start, goal});
        return Result::Direct;
    }
    m_drone.setRoute(buildDetour());
    return Result::Detour;
}

std::vector<Point3D> Environment::buildDetour() const
{
    const Point3D from = m_start - m_zone.center();
    const Point3D to = m_goal - m_zone.center();
    const double startDistance = length(from);
    const double goalDistance = length(to);
    const Point3D u = from * (1 / startDistance);
    Point3D v = to - u * dot(to, u);
    if (length(v) < 1e-8) {
        const Point3D axis = std::abs(u.z) < 0.9 ? Point3D{0, 0, 1} : Point3D{0, 1, 0};
        v = axis - u * dot(axis, u);
    }
    v = v * (1 / length(v));

    const double radius = m_zone.radius() + Drone::radius + clearance;
    const double goalAngle = std::atan2(dot(to, v), dot(to, u));
    const double startTangent = std::acos(std::clamp(radius / startDistance, 0.0, 1.0));
    const double goalTangent = std::acos(std::clamp(radius / goalDistance, 0.0, 1.0));
    const double maxAngle = std::min(pi / 36,
        2 * std::acos((radius - clearance / 2) / radius));

    std::vector<Point3D> best;
    for (int direction : {1, -1}) {
        const double firstAngle = direction * startTangent;
        const double lastAngle = goalAngle - direction * goalTangent;
        double sweep = std::fmod(direction * (lastAngle - firstAngle) + 2 * pi, 2 * pi);
        const int count = std::max(1, int(std::ceil(sweep / maxAngle)));
        std::vector<Point3D> points{m_start};
        for (int i = 0; i <= count; ++i) {
            const double angle = firstAngle + direction * sweep * i / count;
            points.push_back(m_zone.center()
                             + u * (radius * std::cos(angle))
                             + v * (radius * std::sin(angle)));
        }
        points.push_back(m_goal);
        if (best.empty() || pathLength(points) < pathLength(best))
            best = points;
    }
    return best;
}

double Environment::routeLength() const
{
    return pathLength(m_drone.route());
}
