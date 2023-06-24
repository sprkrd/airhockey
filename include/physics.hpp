#pragma once

#include "vector_maths.hpp"
#include "geometry.hpp"

#include <array>
#include <limits>
#include <memory>
#include <optional>


namespace ash {

constexpr double inf = std::numeric_limits<double>::infinity();

enum class Body_type : uint64_t {Box, Disk};
constexpr size_t number_of_body_types = 2;

namespace parameters {

constexpr int substeps = 10;
constexpr double dt = 0.02;
constexpr double substep_dt = dt/substeps;
constexpr double slop = 1e-6;
constexpr double positional_correction = 0.5;
constexpr double field_length = 1.948;
constexpr double field_width = 1.038;
constexpr double goal_width = 0.25;
constexpr double mallet_radius = 0.04815;
constexpr double mallet_mass = 0.17;
constexpr double puck_radius = 0.03165;
constexpr double puck_mass = 0.015;
constexpr double puck_mu = 0.01;
constexpr double gravity = 9.81;
constexpr double mallet_max_velocity = 4;
constexpr double mallet_wall_restitution = 0.2;
constexpr double mallet_puck_restitution = 0.6;
constexpr double puck_wall_restitution = 0.9;
constexpr double wall_thickness = 0.02;
constexpr double kp = 150;
constexpr double kd = 10;

}

class Body {
    public:

        typedef std::unique_ptr<Body> Ptr;
        typedef std::unique_ptr<const Body> ConstPtr;

        Body(double mass = 1) : inv_mass(1.0 / mass) {
        }

        Body& set_position(const Vector_2d& position) {
            this->position = position;
            return *this;
        }

        Body& set_velocity(const Vector_2d& velocity) {
            this->velocity = velocity;
            return *this;
        }

        Body& apply_force(const Vector_2d& force) {
            this->force += force;
            return *this;
        }

        Body& reset_force() {
            force.x = force.y = 0;
            return *this;
        }

        const Vector_2d& get_position() const {
            return position;
        }

        const Vector_2d& get_velocity() const {
            return velocity;
        }

        const Vector_2d& get_force() const {
            return force;
        }

        double get_mass() const {
            return inv_mass==0? inf : (1.0/inv_mass);
        }

        double get_inv_mass() const {
            return inv_mass;
        }

        virtual Body_type get_type() const = 0;

        virtual AABB get_bounding_box() const = 0;

        virtual ~Body() = default;

    private:
        Vector_2d position;
        Vector_2d velocity;
        Vector_2d force;
        double inv_mass;
};

class Box : public Body {
    public:
        Box(const Vector_2d& size = Vector_2d(1,1), double mass = 1) :
            Body(mass), size(size) {}

        const Vector_2d& get_size() const {
            return size;
        }

        Body_type get_type() const override {
            return Body_type::Box;
        }

        AABB get_bounding_box() const override;

    private:
        Vector_2d size;
};

class Disk : public Body {
    public:
        Disk(double radius = 1, double mass = 1) :
            Body(mass), radius(radius) {
        }

        double get_radius() const {
            return radius;
        }

        Body_type get_type() const override {
            return Body_type::Disk;
        }

        AABB get_bounding_box() const override;

    private:
        double radius;

};

struct Collision {
    Body* a;
    Body* b;
    Vector_2d normal;
    double penetration;
};

bool broadphase_test(const Body& a, const Body& b);

std::optional<Collision> collides(Body& a, Body& b);

void resolve_collision(Collision& collision, double restitution = 1.0);

void correct_position(
        Collision& collision, double slop, double amount);

class Environment {
    public:
        struct State {
            struct BodyStatus {
                Vector_2d position;
                Vector_2d velocity;
            };

            std::array<BodyStatus,2> mallets;
            BodyStatus puck;
        };
        typedef Vector_2d Action;
        typedef std::array<Box,6> Wall_array;
        typedef std::array<Box,3> Barrier_array;
        typedef std::array<Disk,2> Mallet_array;
        typedef std::array<int,2> Score_array;

        Environment();

        const Wall_array& get_walls() const {
            return walls;
        }

        const Barrier_array& get_barriers() const {
            return barriers;
        }

        const Mallet_array& get_mallets() const {
            return mallets;
        }

        const Disk& get_puck() const {
            return puck;
        }

        void reset(size_t sender);

        int step(const Action& a1, const Action& a2);

        State get_state() const;

        void set_state(const State& state);

    private:

        void handle_collision(Body& a, Body& b, double restitution);

        int substep(const Action& a1, const Action& a2);

    public:
        Wall_array walls;
        Barrier_array barriers;
        Mallet_array mallets;
        Disk puck;
        
};

}
