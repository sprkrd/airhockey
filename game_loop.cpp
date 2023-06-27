#include "game_loop.hpp"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>


class ash::Game_loop::PlayerImpl {
    public:

        PlayerImpl(size_t index = 0) : index(index) {
        }

        size_t get_index() const {
            return index;
        }
        
        virtual ash::Vector_2d get_input() = 0;

        virtual void report_state(const Game_state& state) = 0;

        virtual ~PlayerImpl() = default;

    protected:

        void set_index(size_t index) {
            this->index = index;
        }

    private:

        size_t index;

};

namespace {


const char* status_str(sf::Socket::Status status) {
    switch (status) {
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
    return "";
}


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

namespace Packet_type {

constexpr sf::Int32 player_index = 0;
constexpr sf::Int32 state_report = 1;
constexpr sf::Int32 input_report = 2;
constexpr sf::Int32 shutdown = 3;

}

sf::Packet& operator<<(sf::Packet& packet, const ash::Vector_2d& v) {
    return packet << v.x << v.y;
}

sf::Packet& operator>>(sf::Packet& packet, ash::Vector_2d& v) {
    return packet >> v.x >> v.y;
}


sf::Packet& operator<<(
        sf::Packet& packet,
        const ash::Environment::State::BodyStatus& status) {
    return packet << status.position << status.velocity;
}

sf::Packet& operator>>(
        sf::Packet& packet,
        ash::Environment::State::BodyStatus& status) {
    return packet >> status.position >> status.velocity;
}

sf::Packet& operator<<(
        sf::Packet& packet,
        const ash::Environment::State& state) {
    return packet << state.mallets[0] << state.mallets[1]
                  << state.puck;
}

sf::Packet& operator>>(
        sf::Packet& packet,
        ash::Environment::State& state) {
    return packet >> state.mallets[0] >> state.mallets[1]
                  >> state.puck;
}


sf::Packet& operator<<(
        sf::Packet& packet,
        const ash::Game_state& state) {
    packet << state.environment.get_state()
           << sf::Int32(state.score[0]) << sf::Int32(state.score[1])
           << sf::Int32(state.sender)
           << sf::Int32(state.new_game);
    return packet;
}


sf::Packet& operator>>(
        sf::Packet& packet,
        ash::Game_state& state) {
    using namespace ash::parameters;
    ash::Environment::State env_state;
    sf::Int32 score_0, score_1, sender, new_game;
    packet >> env_state
           >> score_0 >> score_1
           >> sender
           >> new_game;
    state.environment.set_state(env_state);
    state.score[0] = score_0;
    state.score[1] = score_1;
    state.sender = sender;
    state.new_game = new_game;
    return packet;
}

class RemotePlayer {
    public:

        ~RemotePlayer() {
            sf::Packet packet;
            packet << Packet_type::shutdown;
            send_packet(packet);
        }

    protected:

        void throw_error(const std::string& msg = "Socket error") {
            std::ostringstream oss;
            oss << msg << ". Socket status: " << status_str(status);
            throw std::runtime_error(oss.str());
        }

        void process(sf::Packet& packet, bool send, double timeout_s) {
            sf::Time timeout = sf::seconds(timeout_s);
            sf::Clock clk;
            bool onprogress;
            bool timed_out;
            do {
                if (send) {
                    status = remote.send(packet);
                }
                else {
                    status = remote.receive(packet);
                }
                onprogress = status == sf::Socket::Partial ||
                    status == sf::Socket::NotReady;
                timed_out = clk.getElapsedTime() >= timeout;

            } while (onprogress && !timed_out);
            if (timed_out) {
                throw_error("Timeout");
            }
            else if (status != sf::Socket::Done) {
                throw_error();
            }
        }

        void send_packet(sf::Packet& packet, double timeout_s = 10e-3) {
            process(packet, true, timeout_s);
        }

        sf::Packet receive_packet(double timeout_s = 10e-3) {
            sf::Packet packet;
            process(packet, false, timeout_s);
            return packet;
        }

        sf::TcpSocket remote;
        sf::Socket::Status status; 
};

class RemoteClient : public ash::Game_loop::PlayerImpl, RemotePlayer {
    public:
        RemoteClient(size_t index, unsigned short port) :
            PlayerImpl(index) 
        {
            sf::TcpListener listener;
            status = listener.listen(port);
            if (status != sf::Socket::Done) {
                throw_error();
            }
            status = listener.accept(remote);
            if (status != sf::Socket::Done) {
                throw_error();
            }
            remote.setBlocking(false);
            sf::Packet packet;
            packet << Packet_type::player_index << sf::Int32(get_index());
            send_packet(packet);
        }

        ash::Vector_2d get_input() override {
            ash::Vector_2d input;
            auto packet = receive_packet();
            sf::Int32 packet_type;
            packet >> packet_type;
            if (packet_type != Packet_type::input_report) {
                throw_error("Expecting input report");
            }
            packet >> input;
            return input;
       }

        void report_state(const ash::Game_state& state) override {
            sf::Packet packet;
            packet << Packet_type::state_report << state; 
            send_packet(packet);
        }
};


}

class ash::Game_loop::Remote_server : public RemotePlayer {
    public:
        Remote_server(const std::string& address,
                unsigned short port) {
            using ::operator>>;
            status = remote.connect(address, port);
            if (status != sf::Socket::Done) {
                throw_error();
            }
            remote.setBlocking(false);
            auto packet = receive_packet(100e-3);
            sf::Int32 packet_type, player_index;
            packet >> packet_type;
            if (packet_type != Packet_type::player_index) {
                throw_error("Received wrong packet type"
                        ", expecting player index");
                
            }
            packet >> player_index;
            local_player = player_index;
        }

        void step(ash::Game_state& state,
                const ash::Environment::Action& a) {
            using ::operator>>;
            using ::operator<<;
            sf::Packet input;
            input << Packet_type::input_report << a;
            send_packet(input);
            auto response = receive_packet();
            sf::Int32 packet_type;
            response >> packet_type;
            if (packet_type != Packet_type::state_report) {
                throw_error("Expecting state report");
            }
            response >> state;
        }

        int get_local_player() {
            return local_player;
        }

    private:
        int local_player;
};


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

void ash::Game_loop::set_remote_client(size_t player, unsigned short port) {
    std::cout << "Waiting for client..." << std::endl;
    players[player].reset(new RemoteClient(player, port));
    std::cout << "Client connected" << std::endl;
}

void ash::Game_loop::set_remote_server(const std::string& address, unsigned short port) {
    std::cout << "Connecting to server..." << std::endl;
    remote_server.reset(new Remote_server(address, port));
    std::cout << "Connected to server" << std::endl;
    set_local_player(remote_server->get_local_player());
}

void ash::Game_loop::run() {
    start_new_game(0);
    report_state_to_players();
    while (window->isOpen()) {
        process_events();
        update_game_state();
        render(true);
    }
}

ash::Game_loop::~Game_loop() = default;

void ash::Game_loop::start_new_game(int sender) {
    game_state.sender = sender;
    game_state.environment.reset(sender);
    game_state.new_game = true;
    game_state.accumulator = 0;
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
    game_state.accumulator += clk.restart().asSeconds();
    while (game_state.accumulator > parameters::dt) {
        auto input1 = players[0]->get_input();
        auto input2 = players[1]->get_input();
        int winner = game_state.environment.step(input1, input2);
        if (winner != -1) {
            ++game_state.score[winner];
            start_new_game(1 - game_state.sender);
        }
        else {
            game_state.new_game = false;
            game_state.accumulator -= parameters::dt;
        }
        report_state_to_players();
    }
}

void ash::Game_loop::report_state_to_players() {
    for (size_t i = 0; i < 2; ++i) {
        players[i]->report_state(game_state);
    }
}

void ash::Game_loop::draw_body(const Body& body, const sf::Color& color,
        bool extrapolate) {
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
    double x, y;
    if (extrapolate) {
        auto position = body.get_position() +
            body.get_velocity()*game_state.accumulator;
        x = position.x;
        y = -position.y;
    }
    else {
        x = body.get_position().x;
        y = -body.get_position().y;
    }
    shape->setPosition(x, y);
    shape->setFillColor(color);
    window->draw(*shape);
}

void ash::Game_loop::render(bool extrapolate) {
    window->clear(sf::Color::Black);
    window->setView(game_view);
    const auto& env = game_state.environment;
    for (const auto& wall : env.get_walls()) {
        draw_body(wall, sf::Color(250, 100, 50), false);
    }
    for (const auto& barrier : env.get_barriers()) {
        draw_body(barrier, sf::Color(255, 255, 255, 32), false);
    }
    for (const auto& mallet : env.get_mallets()) {
        draw_body(mallet, sf::Color(100, 250, 50), extrapolate);
    }
    draw_body(env.get_puck(), sf::Color(100, 50, 250), extrapolate);
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

