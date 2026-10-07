#ifndef ZONE_H
#define ZONE_H

#include "point3d.h"

class Zone
{
public:
    Zone(const Point3D &center = {300, 200, 200}, double radius = 120);

    Point3D center() const { return m_center; }
    double radius() const { return m_radius; }
    bool contains(const Point3D &point, double margin = 0) const;
    double distanceToSegment(const Point3D &from, const Point3D &to) const;
    bool intersects(const Point3D &from, const Point3D &to, double margin = 0) const;

private:
    Point3D m_center;
    double m_radius;
};

#endif
