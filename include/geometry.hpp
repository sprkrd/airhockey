#pragma once

#include "vector_maths.hpp"
#include <limits>

namespace ash {

struct AABB {
    double x_min;
    double x_max;
    double y_min;
    double y_max;

    bool intersects(const AABB& other) const;

    bool contains(const Vector_2d& point) const;

};

}
