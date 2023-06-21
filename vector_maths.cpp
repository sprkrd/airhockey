#include "vector_maths.hpp"


std::ostream& ash::operator<<(std::ostream& out, const Vector_2d& v) {
    return out << '(' << v.x << ',' << v.y << ')';
}
