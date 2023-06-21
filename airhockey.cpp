#include "physics.hpp"
#include "viz.hpp"
#include "player.hpp"

#include <SFML/Graphics.hpp>

#include <cassert>
#include <iostream>


int main(int args, char* argv[]) {
    using ::ash::parameters::dt;

    bool vsync = false;

    sf::RenderWindow win(sf::VideoMode(600, 480), "Air Hockey");
    win.setVerticalSyncEnabled(false);

    ash::Environment env;

    ash::Renderer renderer(env, win);
    renderer.reset_mouse(0);

    ash::Player::Ptr player1 = std::make_unique<ash::Local_player>(renderer);
    std::cout << "Waiting for player 2..." << std::endl;
    ash::Player::Ptr player2 = std::make_unique<ash::Remote_player>();
    std::cout << "Player 2 connected" << std::endl;

    sf::Cursor cursor;
    cursor.loadFromSystem(sf::Cursor::Cross);
    win.setMouseCursor(cursor);

    size_t sender = 0;

    auto current_state = env.get_state();
    player1->report_state(current_state);
    player2->report_state(current_state);

    sf::Clock clock;
    double accumulator = 0.0;
    while (win.isOpen()) {
        // process event(s)
        sf::Event event;
        while (win.pollEvent(event)) {
            switch (event.type) {
                case sf::Event::Resized: {
                    renderer.set_view(
                            event.size.width, event.size.height, 1.0);
                    break;
                }
                case sf::Event::Closed: {
                    win.close();
                    break;
                }
                case sf::Event::KeyPressed: {
                    if (event.key.code == sf::Keyboard::Escape) {
                        win.close();
                    }
                    else if (event.key.code == sf::Keyboard::V) {
                        vsync = !vsync;
                        win.setVerticalSyncEnabled(vsync);
                    }
                    break;
                }
                case sf::Event::KeyReleased: {
                    break;
                }
                case sf::Event::MouseMoved: {
                    break;
                }
                default: {
                    break;
                }
            }
        }

        accumulator += clock.restart().asSeconds();

        while (accumulator >= dt) {
            auto action_p1 = player1->get_next_action();
            auto action_p2 = player2->get_next_action();
            int result = env.step(action_p1, action_p2);
            if (result != 0) {
                sender = 1 - sender;
                env.reset(sender);
                renderer.reset_mouse(0);
            }
            current_state = env.get_state();
            player1->report_state(current_state);
            player2->report_state(current_state);
            accumulator -= dt;
        }

        renderer.render();

    }
}

