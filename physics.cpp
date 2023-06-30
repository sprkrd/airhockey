#include "physics.hpp"

#include <cassert>


namespace {

inline constexpr auto merge(ash::Body_type l, ash::Body_type r) {
    using Body_index = std::underlying_type_t<ash::Body_type>;
    return static_cast<Body_index>(l)*ash::number_of_body_types +
        static_cast<Body_index>(r);
}

std::optional<ash::Collision> collides_box_vs_box (
        ash::Body& base_a, ash::Body& base_b) {
    auto* a = static_cast<ash::Box*>(&base_a);
    auto* b = static_cast<ash::Box*>(&base_b);
    const auto& bb_a = a->get_bounding_box();
    const auto& bb_b = b->get_bounding_box();

    if (bb_a.intersects(bb_b)) {
        double pen_right = bb_a.x_max - bb_b.x_min;
        double pen_left = bb_b.x_max - bb_a.x_min;
        double pen_up = bb_a.y_max - bb_b.y_min;
        double pen_down = bb_b.y_max - bb_a.y_min;
        ash::Collision out;
        out.a = a;
        out.b = b;
        out.normal = ash::Vector_2d(1,0);
        out.penetration = pen_right; 
        if (pen_left < out.penetration) {
            out.normal = ash::Vector_2d(-1, 0);
            out.penetration = pen_left;
        }
        if (pen_up < out.penetration) {
            out.normal = ash::Vector_2d(0, 1);
            out.penetration = pen_up;
        }
        if (pen_down < out.penetration) {
            out.normal = ash::Vector_2d(0, -1);
            out.penetration = pen_down;
        }
        return out;
    }
    return {};
}

std::optional<ash::Collision> collides_box_vs_disk(
        ash::Body& base_a, ash::Body& base_b) {
    // (xmin,ymax)       (xmax,ymax)
    //            +-----+
    //            |\ B /|  . P(x,y) 
    //            | \ / |
    //            |A X C|
    //            | / \ |
    //            |/ D \|
    //            +-----+
    // (xmin,ymin)       (xmax,ymin)
    //
    //
    // v1 = (xmax-xmin,ymax-ymin)
    // v2 = (xmax-xmin,ymin-ymax)
    // u1 = (x-xmin,y-ymin)
    // u2 = (x-xmin,y-ymax)
    
    auto* a = static_cast<ash::Box*>(&base_a);
    auto* b = static_cast<ash::Disk*>(&base_b);
    const auto& bb_a = a->get_bounding_box();
    const auto& pos_b = b->get_position();
    double radius = b->get_radius();

    ash::Vector_2d v1(bb_a.x_max-bb_a.x_min, bb_a.y_max-bb_a.y_min);
    ash::Vector_2d v2(bb_a.x_max-bb_a.x_min, bb_a.y_min-bb_a.y_max);
    ash::Vector_2d u1(pos_b.x-bb_a.x_min, pos_b.y-bb_a.y_min);
    ash::Vector_2d u2(pos_b.x-bb_a.x_min, pos_b.y-bb_a.y_max);

    bool ab = v1.x*u1.y - v1.y*u1.x > 0;
    bool bc = v2.x*u2.y - v2.y*u2.x > 0;

    ash::Vector_2d closest;
    ash::Vector_2d normal;

    if (!ab && !bc) {
        // bottom border
        closest = ash::Vector_2d(
                ash::clamp(pos_b.x, bb_a.x_min, bb_a.x_max), bb_a.y_min);
        normal = ash::Vector_2d(0, -1);
    }
    else if (!ab && bc) {
        // right border
        closest = ash::Vector_2d(
                bb_a.x_max, ash::clamp(pos_b.y, bb_a.y_min, bb_a.y_max));
        normal = ash::Vector_2d(1, 0);
    }
    else if (ab && !bc) {
        // left border
        closest = ash::Vector_2d(
                bb_a.x_min, ash::clamp(pos_b.y, bb_a.y_min, bb_a.y_max));
        normal = ash::Vector_2d(-1, 0);
    }
    else { 
        // top border
        closest = ash::Vector_2d(
                ash::clamp(pos_b.x, bb_a.x_min, bb_a.x_max), bb_a.y_max);
        normal = ash::Vector_2d(0, 1);
    }
    auto closest_to_b = pos_b - closest;
    double distance = closest_to_b.norm_sq();
    bool inside = bb_a.contains(pos_b);
    if (inside || distance < radius*radius) {
        distance = sqrt(distance);
        ash::Collision out;
        out.a = a;
        out.b = b;
        out.normal = distance==0? normal : (closest_to_b/distance);
        out.penetration = inside? (radius + distance) :
            (radius - distance);
        return out;
    }

    return {};
}

std::optional<ash::Collision> collides_disk_vs_disk(
        ash::Body& base_a, ash::Body& base_b) {
    auto* a = static_cast<ash::Disk*>(&base_a);
    auto* b = static_cast<ash::Disk*>(&base_b);
    double sum_radius = a->get_radius() + b->get_radius();
    auto displacement = b->get_position() - a->get_position();
    double distance_sq = displacement.norm_sq();
    if (distance_sq < sum_radius*sum_radius) {
        double distance = sqrt(distance_sq);
        ash::Collision out;
        out.a = a;
        out.b = b;
        out.penetration = sum_radius - distance;
        out.normal = distance_sq == 0?
            ash::Vector_2d(1,0) :
            displacement/distance;
        return out;
    }
    return {};
}

ash::Vector_2d pd_control(const ash::Vector_2d& t, const ash::Body& b) {
    using namespace ::ash::parameters;
    constexpr double max_p_force = kd*mallet_max_velocity;
    auto p = kp*(t-b.get_position());
    auto p_mag_sq = p.norm_sq();
    if (p_mag_sq > max_p_force*max_p_force) {
        p = max_p_force/sqrt(p_mag_sq) * p;
    }
    auto d = - kd*b.get_velocity();
    return p + d;
}

}


ash::AABB ash::Disk::get_bounding_box() const {
    const auto& position = get_position();
    AABB result;
    result.x_min = position.x - radius;
    result.x_max = position.x + radius;
    result.y_min = position.y - radius;
    result.y_max = position.y + radius;
    return result;
}

ash::AABB ash::Box::get_bounding_box() const {
    const auto& position = get_position();
    AABB result;
    result.x_min = position.x - size.x/2;
    result.x_max = position.x + size.x/2;
    result.y_min = position.y - size.y/2;
    result.y_max = position.y + size.y/2;
    return result;
}

bool ash::broadphase_test(const Body& a, const Body& b) {
    auto bb_a = a.get_bounding_box();
    auto bb_b = b.get_bounding_box();
    return bb_a.intersects(bb_b);
}

std::optional<ash::Collision> ash::collides(Body& a, Body& b) {
    switch (merge(a.get_type(),b.get_type())) {
        case merge(Body_type::Box,  Body_type::Box):
            return collides_box_vs_box(a, b);
        case merge(Body_type::Box,  Body_type::Disk):
            return collides_box_vs_disk(a, b);
        case merge(Body_type::Disk, Body_type::Box):
            return collides_box_vs_disk(b, a);
        case merge(Body_type::Disk, Body_type::Disk):
            return collides_disk_vs_disk(a, b);
        default:
            assert(false);
    }
    return {};
}

void ash::resolve_collision(Collision& collision, double restitution) {
    auto& a = *collision.a;
    auto& b = *collision.b;
    auto v_a = a.get_velocity();
    auto v_b = b.get_velocity();
    auto v_ab_n = collision.normal.dot(v_b - v_a);
    if (v_ab_n < 0) {
        double den = a.get_inv_mass()+b.get_inv_mass();
        double w1 = a.get_inv_mass() / den;
        double w2 = b.get_inv_mass() / den;
        v_a += (w1*(1+restitution)*v_ab_n) * collision.normal;
        v_b -= (w2*(1+restitution)*v_ab_n) * collision.normal;
        a.set_velocity(v_a);
        b.set_velocity(v_b);
    }
}

void ash::correct_position(
        Collision& collision, double slop, double amount) {
    if (collision.penetration < slop) {
        return;
    }
    auto& a = *collision.a;
    auto& b = *collision.b;
    auto pos_a = a.get_position();
    auto pos_b = b.get_position();
    double den = a.get_inv_mass() + b.get_inv_mass();
    double w1 = a.get_inv_mass() / den;
    double w2 = b.get_inv_mass() / den;
    pos_a -= w1*amount*collision.penetration * collision.normal;
    pos_b += w2*amount*collision.penetration * collision.normal;
    a.set_position(pos_a);
    b.set_position(pos_b);
}

ash::Environment::Environment() {
    using namespace ::ash::parameters;

    // construct walls
    for (size_t i = 0; i < 6; ++i) {
        double w, h, x, y;
        if (i < 2) {
            double d = field_width +
                wall_thickness;
            w = field_length + 2*wall_thickness;
            h = wall_thickness;
            x = 0;
            y = d/2 - i*d;
        }
        else {
            double d = field_length +
                wall_thickness;
            double e = (field_width + goal_width)/2;
            w = wall_thickness;
            h = (field_width - goal_width)/2;
            x = -d/2 + (i%2)*d;
            y = e/2 - (i>3)*e;
        }
        walls[i] = Box(Vector_2d(w,h), inf);
        walls[i].set_position(Vector_2d(x,y));
    }

    // construct barriers
    for (size_t i = 0; i < 2; ++i) {
        double w = wall_thickness;
        double h = goal_width;
        double x = (field_length + wall_thickness)*(2*i - 1.0)/2;
        barriers[i] = Box(Vector_2d(w,h), inf);
        barriers[i].set_position(Vector_2d(x,0));
    }
    barriers[2] = Box(Vector_2d(wall_thickness,field_width), inf);

    // construct mallet and puck
    for (size_t i = 0; i < 2; ++i) {
        mallets[i] = Disk(
                mallet_radius, mallet_mass);
    }

    puck = Disk(puck_radius, puck_mass);

    // reset mallet and puck position
    reset(0);

}

void ash::Environment::reset(size_t sender) {
    using namespace ::ash::parameters;
    double d = field_length - 2*mallet_radius;
    // reset mallet positions
    for (size_t i = 0; i < 2; ++i) {
        double x = -d/2 + i*d;
        mallets[i].set_position(Vector_2d(x,0));
        mallets[i].set_velocity(Vector_2d(0,0));
    }
    // reset puck position
    {
        double e = field_length/2;
        double x = -e/2 + sender*e;
        puck.set_position(Vector_2d(x, 0));
        puck.set_velocity(Vector_2d(0, 0));
    }
}

int ash::Environment::step(const Action& a1, const Action& a2) {
    // apply forces to mallets
    using namespace ::ash::parameters;
    for (int i = 0; i < substeps-1; ++i) {
        substep(a1, a2);
    }
    return substep(a1, a2);
}

ash::Environment::State ash::Environment::get_state() const {
    State state;
    for (size_t i = 0; i < mallets.size(); ++i) {
        state.mallets[i].position = mallets[i].get_position();
        state.mallets[i].velocity = mallets[i].get_velocity();
    }
    state.puck.position = puck.get_position();
    state.puck.velocity = puck.get_velocity();
    return state;
}

void ash::Environment::set_state(const State& state) {
    for (size_t i = 0; i < mallets.size(); ++i) {
        mallets[i].set_position(state.mallets[i].position);
        mallets[i].set_velocity(state.mallets[i].velocity);
    }
    puck.set_position(state.puck.position);
    puck.set_velocity(state.puck.velocity);
}

int ash::Environment::substep(const Action& a1, const Action& a2) {
    // apply forces to mallets
    using namespace ::ash::parameters;

    Vector_2d force_1 = pd_control(a1, mallets[0]);
    Vector_2d force_2 = pd_control(a2, mallets[1]);

    mallets[0].apply_force(force_1);
    mallets[1].apply_force(force_2); 

    // Semi-implicit or symplectic Euler integration
    Body* bodies[] = {&mallets[0], &mallets[1], &puck};
    for (auto& body : bodies) {
        auto position = body->get_position();
        auto velocity = body->get_velocity();

        // 1.2. friction (only_puck)
        double velocity_mag = velocity.norm();
        if (velocity_mag>0 && body==&puck) {
            double velocity_dec = gravity*puck_mu*substep_dt;
            velocity_dec = fmin(velocity_mag, velocity_dec);
            velocity -= velocity_dec/velocity_mag * velocity;
        }

        // 1. update velocity
        // 1.1. accumulated force
        velocity += body->get_force()*body->get_inv_mass()*substep_dt;
 


        // 2. update position
        position += velocity*substep_dt;

        // 3. update body's internal state, reset force
        body->set_position(position);
        body->set_velocity(velocity);
        body->reset_force();

    }

    // detect and resolve collisions
    for (auto& wall : walls) {
        for (auto& mallet : mallets) {
            handle_collision(wall, mallet, mallet_wall_restitution);
        }
    }
    for (auto& barrier : barriers) {
        for (auto& mallet : mallets) {
            handle_collision(barrier, mallet, mallet_wall_restitution);
        }
    }
    for (auto& wall : walls) {
        handle_collision(wall, puck, puck_wall_restitution);
    }
    for (auto& mallet : mallets) {
        handle_collision(mallet, puck, mallet_puck_restitution);
    }

    // check if puck has entered one of the goals
    double goal_threshold = field_length/2 + puck_radius;
    if (puck.get_position().x < -goal_threshold) {
        return 1;
    }
    if (puck.get_position().x > goal_threshold) {
        return 0;
    }
    return -1;
}

void ash::Environment::handle_collision(Body& a, Body& b,
        double restitution) {
    using namespace ::ash::parameters;
    if (!broadphase_test(a, b)) {
        return;
    }
    if (auto collision = collides(a, b)) {
        resolve_collision(*collision, restitution);
        correct_position(*collision, slop, positional_correction);
    }
}

