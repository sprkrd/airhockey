#pragma once

#include <cmath>
#include <ostream>


namespace ash {

inline constexpr double clamp(double x, double lo, double hi) {
    if (x < lo) {
        x = lo;
    }
    else if (x > hi) {
        x = hi;
    }
    return x;

}

struct Vector_2d {
    double x;
    double y;

    constexpr Vector_2d(double x = 0, double y = 0) : x(x), y(y) {}

    constexpr bool is_zero() const {
        return x==0 && y==0;
    }

    constexpr bool operator==(const Vector_2d& other) const {
        return x==other.x && y==other.y;
    }

    constexpr bool operator!=(const Vector_2d& other) const {
        return x!=other.x && y!=other.y;
    }

    constexpr Vector_2d operator+(const Vector_2d& other) const {
        return Vector_2d{x+other.x, y+other.y};
    }

    Vector_2d& operator+=(const Vector_2d& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr Vector_2d operator-(const Vector_2d& other) const {
        return Vector_2d(x-other.x, y-other.y);
    }

    constexpr Vector_2d operator-() const {
        return Vector_2d(-x,-y);
    }

    Vector_2d& operator-=(const Vector_2d& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr Vector_2d operator*(double s) const {
        return Vector_2d(x*s, y*s);
    }

    Vector_2d& operator*=(double s) {
        x *= s;
        y *= s;
        return *this;
    }

    constexpr Vector_2d operator/(double s) const {
        return Vector_2d{x/s, y/s};
    }

    Vector_2d& operator/=(double s) {
        x /= s;
        y /= s;
        return *this;
    }

    constexpr double norm_sq() const {
        return x*x + y*y;
    }

    constexpr double norm() const {
        return sqrt(norm_sq());
    }

    constexpr Vector_2d normalize() const {
        return (*this) / norm();
    }

    constexpr double dot(const Vector_2d& other) {
        return x*other.x + y*other.y;
    }

    constexpr double angle(const Vector_2d& other) {
        return acos(dot(other) / (norm()*other.norm()));
    } 
};

inline constexpr Vector_2d operator*(double s, const Vector_2d& v) {
    return v*s;
}

std::ostream& operator<<(std::ostream& out, const Vector_2d& v);

}
