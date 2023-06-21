#include "viz.hpp"

#include <iostream>
#include <cassert>
#include <iomanip>
#include <memory>
#include <sstream>


ash::Renderer::Renderer(const Environment& env, sf::RenderWindow& win) :
    env(env),
    win(win)
{
    auto[win_x, win_y] = win.getSize();
    font.loadFromFile(fontfile);
    set_view(win_x, win_y);
}

void ash::Renderer::set_view(size_t w, size_t h, double zoom) {
    using namespace ::ash::parameters;
    double aspect_ratio = ((double)w)/h;
    game_view = sf::View(
        sf::Vector2f(0,0),
        sf::Vector2f(field_length*1.25/zoom,
            field_length*1.25/(aspect_ratio*zoom))
    );
    ui_view = sf::View(sf::FloatRect(0, 0, w, h));
}

ash::Vector_2d ash::Renderer::window_to_world_coordinates(
        const sf::Vector2i& win_coords) const {
    auto[x,y] = win.mapPixelToCoords(win_coords, game_view);
    return Vector_2d(x, -y);
}

sf::Vector2i ash::Renderer::world_to_window_coordinates(
        const Vector_2d& world_coords) const {
    sf::Vector2f sf_world_coords(world_coords.x, -world_coords.y);
    return win.mapCoordsToPixel(sf_world_coords, game_view);
}

ash::Vector_2d ash::Renderer::get_mouse_in_world() const {
    return window_to_world_coordinates(sf::Mouse::getPosition(win));
}

void ash::Renderer::reset_mouse(size_t player) const {
    const auto& world_coords = env.get_mallets()[player]
        .get_position();
    auto mouse_coords = world_to_window_coordinates(world_coords);
    sf::Mouse::setPosition(mouse_coords, win);
}

void ash::Renderer::render() {
    win.clear(sf::Color::Black);
    draw_scene();
    win.display();
}

void ash::Renderer::draw_body(const Body& body, const sf::Color& color) {
    std::unique_ptr<sf::Shape> shape;
    switch (body.get_type()) {
        case ash::Body_type::Box: {
            const auto& box = static_cast<const ash::Box&>(body);
            double w = box.get_size().x;
            double h = box.get_size().y;
            shape.reset(new sf::RectangleShape(sf::Vector2f(w,h)));
            shape->setOrigin(w/2,h/2);
            break;
        }
        case ash::Body_type::Disk: {
            const auto& disk = static_cast<const ash::Disk&>(body);
            double r = disk.get_radius();
            shape.reset(new sf::CircleShape(r));
            shape->setOrigin(r,r);
            break;
        }
        default: {
            assert(false);
        }
    }
    double x = body.get_position().x;
    double y = -body.get_position().y;
    shape->setPosition(x, y);
    shape->setFillColor(color);
    win.draw(*shape);
}


void ash::Renderer::draw_score() {
    std::ostringstream oss;
    oss << "Score " << std::setw(3) << env.get_score(0) << ' '
                    << std::setw(3) << env.get_score(1);
    sf::Text score_text(oss.str(), font);
    score_text.setCharacterSize(24);
    score_text.setFillColor(sf::Color::White);
    score_text.setPosition(10,10);
    win.draw(score_text);
}


void ash::Renderer::draw_scene() {
    win.setView(game_view);
    for (const auto& wall : env.get_walls()) {
        draw_body(wall, wall_color);
    }
    for (const auto& barrier : env.get_barriers()) {
        draw_body(barrier, barrier_color);
    }
    for (const auto& mallet : env.get_mallets()) {
        draw_body(mallet, mallet_color);
    }
    draw_body(env.get_puck(), puck_color);
    win.setView(ui_view);
    draw_score();
}

