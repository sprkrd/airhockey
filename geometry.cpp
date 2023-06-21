#include "geometry.hpp"

bool ash::AABB::intersects(const AABB& other) const {
    return x_max > other.x_min && other.x_max > x_min &&
           y_max > other.y_min && other.y_max > y_min;
}

bool ash::AABB::contains(const Vector_2d& point) const {
    return x_min < point.x && point.x < x_max &&
           y_min < point.y && point.y < y_max;
}

