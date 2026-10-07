#ifndef POINT3D_H
#define POINT3D_H

#include <cmath>

struct Point3D
{
    double x = 0;
    double y = 0;
    double z = 0;
};

inline Point3D operator+(const Point3D &a, const Point3D &b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

inline Point3D operator-(const Point3D &a, const Point3D &b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

inline Point3D operator*(const Point3D &p, double factor)
{
    return {p.x * factor, p.y * factor, p.z * factor};
}

inline double dot(const Point3D &a, const Point3D &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline double length(const Point3D &p)
{
    return std::sqrt(dot(p, p));
}

inline double distance(const Point3D &a, const Point3D &b)
{
    return length(a - b);
}

#endif
