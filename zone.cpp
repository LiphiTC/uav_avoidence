#include "zone.h"

#include <algorithm>

Zone::Zone(const Point3D &center, double radius)
    : m_center(center), m_radius(radius)
{
}

bool Zone::contains(const Point3D &point, double margin) const
{
    return distance(point, m_center) <= m_radius + margin;
}

double Zone::distanceToSegment(const Point3D &from, const Point3D &to) const
{
    const Point3D direction = to - from;
    const double squaredLength = dot(direction, direction);
    if (squaredLength < 1e-12)
        return distance(from, m_center);
    const double t = std::clamp(dot(m_center - from, direction) / squaredLength, 0.0, 1.0);
    return distance(from + direction * t, m_center);
}

bool Zone::intersects(const Point3D &from, const Point3D &to, double margin) const
{
    return distanceToSegment(from, to) <= m_radius + margin;
}
