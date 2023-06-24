#include "game_loop.hpp"

#include <cassert>
#include <iomanip>
#include <sstream>


class ash::Game_loop::PlayerImpl {
    public:

        PlayerImpl(size_t index) : index(index) {
        }

        size_t get_index() const {
            return index;
        }
        
        virtual ash::Vector_2d get_input() = 0;

        virtual void report_state(const Game_state& state) = 0;

    private:

        size_t index;

};

namespace {

class NullPlayer : public ash::Game_loop::PlayerImpl {
    public:

        NullPlayer(size_t index) : ash::Game_loop::PlayerImpl(index) {
        }

        ash::Vector_2d get_input() override {
            return next_input;
        }

        void report_state(const ash::Game_state& state) override {
            size_t i = get_index();
            const auto& mallet = state.environment.get_mallets()[i];
            next_input = mallet.get_position();
        }

    private:

        ash::Vector_2d next_input;
};

class LocalPlayer : public ash::Game_loop::PlayerImpl {
    public:
        LocalPlayer(size_t index, const sf::RenderWindow& window,
                const sf::View& game_view) :
            PlayerImpl(index),
            window(window),
            game_view(game_view)
        {

        }

        ash::Vector_2d get_input() override {
            auto mouse_pixel = sf::Mouse::getPosition(window);
            auto[mouse_x, mouse_y] = window.mapPixelToCoords(
                    mouse_pixel, game_view);
            return ash::Vector_2d(mouse_x, -mouse_y);
        }

        void report_state(const ash::Game_state& state) override {
            if (state.new_game) {
                size_t player = get_index();
                auto[mallet_x, mallet_y] = state.environment
                    .get_mallets()[player].get_position();
                sf::Vector2f mallet_coords(mallet_x, mallet_y);
                auto mallet_pixel = window.mapCoordsToPixel(
                        mallet_coords, game_view);
                sf::Mouse::setPosition(mallet_pixel, window);
            }
        }

    private:
        const sf::RenderWindow& window;
        const sf::View& game_view;
};


//class RemoteServer : public ash::Game_loop::PlayerImpl {
//};

class RemoteClient : public ash::Game_loop::PlayerImpl {
    public:
        RemoteClient(size_t index) : PlayerImpl(index) {
        }

    private:
};


}


ash::Game_loop::Game_loop() :
    zoom(1)
{
    window.reset(new sf::RenderWindow(
                sf::VideoMode(600,480), "Airhockey"));
    window->setVerticalSyncEnabled(true);
    reset_view();
    ui_font.loadFromFile("Spaceport_2006.otf");

    for (size_t i = 0; i < 2; ++i) {
        players[i].reset(new NullPlayer(i));
    }
    game_state.score.fill(0);
}

void ash::Game_loop::disable_player(size_t player) {
    players[player].reset(new NullPlayer(player));
}

void ash::Game_loop::set_local_player(size_t player) {
    players[player].reset(new LocalPlayer(player, *window, game_view));
}

void set_remote_client(size_t player) {
    assert(false && "not implemented");
}

void set_remote_server(const sf::String& address, unsigned short port) {
    assert(false && "not implemented");
}

void ash::Game_loop::run() {
    start_new_game(0);
    report_state_to_players();
    while (window->isOpen()) {
        process_events();
        update_game_state();
        render();
    }
}

ash::Game_loop::~Game_loop() = default;

void ash::Game_loop::start_new_game(size_t sender) {
    game_state.sender = sender;
    game_state.environment.reset(sender);
    game_state.new_game = true;
    accumulator = 0;
}

void ash::Game_loop::shutdown() {
    window->close();
}

void ash::Game_loop::reset_view() {
    auto[w,h] = window->getSize();
    double aspect_ratio = ((double)h)/w;
    game_view = sf::View(sf::Vector2f(0,0),
            sf::Vector2f(2.0/zoom,2.0*aspect_ratio/zoom));
    ui_view = sf::View(sf::FloatRect(0, 0, w, h));
}

void ash::Game_loop::process_events() {
    sf::Event evt;
    while (window->pollEvent(evt)) {
        switch (evt.type) {
            case sf::Event::MouseWheelScrolled: {
                float delta = evt.mouseWheelScroll.delta;
                zoom = std::max(0.05, zoom - delta*0.05);
                reset_view();
                break;
            }
            case sf::Event::Resized: {
                reset_view();
                break;
            }
            case sf::Event::Closed: {
                shutdown();
                break;
            }
            case sf::Event::KeyPressed: {
                if (evt.key.code == sf::Keyboard::Key::Escape) {
                    shutdown();
                }
                break;
            }
            default:
                break;
        }
    }
}

void ash::Game_loop::update_game_state() {
    accumulator += clk.restart().asSeconds();
    while (accumulator > parameters::dt) {
        auto input1 = players[0]->get_input();
        auto input2 = players[1]->get_input();
        int winner = game_state.environment.step(input1, input2);
        if (winner != -1) {
            ++game_state.score[winner];
            start_new_game(1 - game_state.sender);
        }
        else {
            game_state.new_game = false;
            accumulator -= parameters::dt;
        }
        report_state_to_players();
    }
}

void ash::Game_loop::report_state_to_players() {
    for (size_t i = 0; i < 2; ++i) {
        players[i]->report_state(game_state);
    }
}

void ash::Game_loop::draw_body(const Body& body, const sf::Color& color) {
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
    window->draw(*shape);
}

void ash::Game_loop::render() {
    window->clear(sf::Color::Black);
    window->setView(game_view);
    const auto& env = game_state.environment;
    for (const auto& wall : env.get_walls()) {
        draw_body(wall, sf::Color(250, 100, 50));
    }
    for (const auto& barrier : env.get_barriers()) {
        draw_body(barrier, sf::Color(255, 255, 255, 32));
    }
    for (const auto& mallet : env.get_mallets()) {
        draw_body(mallet, sf::Color(100, 250, 50));
    }
    draw_body(env.get_puck(), sf::Color(100, 50, 250));
    window->setView(ui_view);
    draw_score();
    window->display();
}

void ash::Game_loop::draw_score() {
    std::ostringstream oss;
    oss << "Score " << std::setw(3) << game_state.score[0] << ' '
                    << std::setw(3) << game_state.score[1];
    sf::Text score_text(oss.str(), ui_font);
    score_text.setCharacterSize(24);
    score_text.setFillColor(sf::Color::White);
    score_text.setPosition(10,10);
    window->draw(score_text);
}

