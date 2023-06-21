#pragma once

#include "physics.hpp"

#include <SFML/Graphics.hpp>

namespace ash {

constexpr const char* fontfile = "Spaceport_2006.otf";
const sf::Color wall_color(250,100,50);
const sf::Color barrier_color(255,255,255,32);
const sf::Color mallet_color(100,250,50);
const sf::Color puck_color(100,50,250);

class Renderer {
    public:
        Renderer(const Environment& env, sf::RenderWindow& win);

        void set_view(size_t w, size_t h, double zoom=1.0);

        Vector_2d window_to_world_coordinates(
                const sf::Vector2i& win_coords) const;

        sf::Vector2i world_to_window_coordinates(
                const Vector_2d& world_coords) const;

        ash::Vector_2d get_mouse_in_world() const;

        void reset_mouse(size_t player) const;

        void render();


    private:

        void draw_body(const Body& body, const sf::Color& color);

        void draw_score();

        void draw_scene();

        const Environment& env;
        sf::Font font;
        sf::RenderWindow& win;
        sf::View game_view;
        sf::View ui_view;

};


}
