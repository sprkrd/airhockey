#include "physics.hpp"
#include "viz.hpp"
#include "player.hpp"

#include <SFML/Graphics.hpp>

#include <cassert>
#include <iostream>


namespace ash {

const char* status_str(sf::Socket::Status status) {
    switch(status) {
        case sf::Socket::Done:
            return "Done";
        case sf::Socket::NotReady:
            return "NotReady";
        case sf::Socket::Partial:
            return "Partial";
        case sf::Socket::Disconnected:
            return "Disconnected";
        case sf::Socket::Error:
            return "Error";
    }
    assert(false);
}

class Remote_environment {
    public:

        Remote_environment() {
            assert(server.connect("localhost", 54000) == sf::Socket::Done);
        }

        ash::Environment::State get_state() {
            ash::Environment::State state;
            sf::Packet packet;
            auto status = server.receive(packet);
            assert(status == sf::Socket::Done);
            packet >> state;
            return state;
        }

        void send_action(const ash::Environment::Action& action) {
            sf::Packet packet;
            packet << action;
            server.send(packet);
        }

    private:

        sf::TcpSocket server;
};

}

int main(int args, char* argv[]) {
    using ::ash::parameters::dt;

    bool vsync = false;

    sf::RenderWindow win(sf::VideoMode(600, 480), "Air Hockey");
    win.setVerticalSyncEnabled(false);

    ash::Environment env;
    ash::Remote_environment remote_env;

    std::cout << "Setting initial state" << std::endl;
    auto state = remote_env.get_state();
    std::cout << "Initial state set" << std::endl;
    env.set_state(state);

    ash::Renderer renderer(env, win);
    renderer.reset_mouse(1);

    ash::Player::Ptr player = std::make_unique<ash::Local_player>(renderer);

    sf::Cursor cursor;
    cursor.loadFromSystem(sf::Cursor::Cross);
    win.setMouseCursor(cursor);

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
                default: {
                    break;
                }
            }
        }

        accumulator += clock.restart().asSeconds();

        while (accumulator >= dt) {
            auto action = player->get_next_action();
            remote_env.send_action(action);
            env.set_state(remote_env.get_state());
            accumulator -= dt;
        }

        renderer.render();

    }
}

