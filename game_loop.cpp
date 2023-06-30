#include "game_loop.hpp"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

enum class Packet_type : sf::Int32 {player_index, state_report,
    input_report, shutdown};

sf::Packet& operator<<(sf::Packet& packet, Packet_type type) {
    return packet << static_cast<sf::Int32>(type);
}

sf::Packet& operator>>(sf::Packet& packet, Packet_type& type) {
    sf::Int32 type_int32;
    packet >> type_int32;
    type = static_cast<Packet_type>(type_int32);
    return packet;
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
         << state.accumulator
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
         >> state.accumulator
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

void send_packet(sf::TcpSocket& socket, sf::Packet& packet,
        sf::Time timeout) {
    sf::Socket::Status status;
    sf::Clock clk;
    bool onprogress;
    bool timed_out;
    do {
        status = socket.send(packet);
        onprogress = status == sf::Socket::Partial ||
            status == sf::Socket::NotReady;
        timed_out = clk.getElapsedTime() >= timeout;

    } while (onprogress && !timed_out);
    if (timed_out) {
      throw ash::Network_error("Time out sending packet");
    }
    else if (status != sf::Socket::Done) {
      throw ash::Network_error("Error sending packet");
    }
}

sf::Packet receive_packet(sf::TcpSocket& socket, sf::Time timeout) {
    sf::Packet packet;
    sf::Socket::Status status;
    sf::Clock clk;
    bool onprogress;
    bool timed_out;
    do {
        status = socket.receive(packet);
        onprogress = status == sf::Socket::Partial ||
            status == sf::Socket::NotReady;
        timed_out = clk.getElapsedTime() >= timeout;

    } while (onprogress && !timed_out);
    if (timed_out) {
      throw ash::Network_error("Time out receiving packet");
    }
    else if (status != sf::Socket::Done) {
      throw ash::Network_error("Error sending packet");
    }
    return packet;
}

//const char* status_str(sf::Socket::Status status) {
    //switch (status) {
        //case sf::Socket::Done:
            //return "Done";
        //case sf::Socket::NotReady:
            //return "NotReady";
        //case sf::Socket::Partial:
            //return "Partial";
        //case sf::Socket::Disconnected:
            //return "Disconnected";
        //case sf::Socket::Error:
            //return "Error";
    //}
    //return "";
//}


//class NullPlayer : public ash::Game_loop::PlayerImpl {
    //public:

        //NullPlayer(size_t index) : ash::Game_loop::PlayerImpl(index) {
        //}

        //ash::Vector_2d get_input() override {
            //return next_input;
        //}

        //void report_state(const ash::Game_state& state) override {
            //size_t i = get_index();
            //const auto& mallet = state.environment.get_mallets()[i];
            //next_input = mallet.get_position();
        //}

    //private:

        //ash::Vector_2d next_input;
//};



//namespace Packet_type {

//constexpr sf::Int32 player_index = 0;
//constexpr sf::Int32 state_report = 1;
//constexpr sf::Int32 input_report = 2;
//constexpr sf::Int32 shutdown = 3;

//}



//class RemotePlayer {
    //public:

        //~RemotePlayer() {
            //sf::Packet packet;
            //packet << Packet_type::shutdown;
            //send_packet(packet);
        //}

    //protected:

        //void throw_error(const std::string& msg = "Socket error") {
            //std::ostringstream oss;
            //oss << msg << ". Socket status: " << status_str(status);
            //throw std::runtime_error(oss.str());
        //}

        //void process(sf::Packet& packet, bool send, double timeout_s) {
            //sf::Time timeout = sf::seconds(timeout_s);
            //sf::Clock clk;
            //bool onprogress;
            //bool timed_out;
            //do {
                //if (send) {
                    //status = remote.send(packet);
                //}
                //else {
                    //status = remote.receive(packet);
                //}
                //onprogress = status == sf::Socket::Partial ||
                    //status == sf::Socket::NotReady;
                //timed_out = clk.getElapsedTime() >= timeout;

            //} while (onprogress && !timed_out);
            //if (timed_out) {
                //throw_error("Timeout");
            //}
            //else if (status != sf::Socket::Done) {
                //throw_error();
            //}
        //}

        //void send_packet(sf::Packet& packet, double timeout_s = 10e-3) {
            //process(packet, true, timeout_s);
        //}

        //sf::Packet receive_packet(double timeout_s = 10e-3) {
            //sf::Packet packet;
            //process(packet, false, timeout_s);
            //return packet;
        //}

        //sf::TcpSocket remote;
        //sf::Socket::Status status; 
//};

//class RemoteClient : public ash::Game_loop::PlayerImpl, RemotePlayer {
    //public:
        //RemoteClient(size_t index, unsigned short port) :
            //PlayerImpl(index) 
        //{
            //sf::TcpListener listener;
            //status = listener.listen(port);
            //if (status != sf::Socket::Done) {
                //throw_error();
            //}
            //status = listener.accept(remote);
            //if (status != sf::Socket::Done) {
                //throw_error();
            //}
            //remote.setBlocking(false);
            //sf::Packet packet;
            //packet << Packet_type::player_index << sf::Int32(get_index());
            //send_packet(packet);
        //}

        //ash::Vector_2d get_input() override {
            //ash::Vector_2d input;
            //auto packet = receive_packet();
            //sf::Int32 packet_type;
            //packet >> packet_type;
            //if (packet_type != Packet_type::input_report) {
                //throw_error("Expecting input report");
            //}
            //packet >> input;
            //return input;
       //}

        //void report_state(const ash::Game_state& state) override {
            //sf::Packet packet;
            //packet << Packet_type::state_report << state; 
            //send_packet(packet);
        //}
//};

}

//class ash::Game_loop::Remote_server : public RemotePlayer {
    //public:
        //Remote_server(const std::string& address,
                //unsigned short port) {
            //using ::operator>>;
            //status = remote.connect(address, port);
            //if (status != sf::Socket::Done) {
                //throw_error();
            //}
            //remote.setBlocking(false);
            //auto packet = receive_packet(100e-3);
            //sf::Int32 packet_type, player_index;
            //packet >> packet_type;
            //if (packet_type != Packet_type::player_index) {
                //throw_error("Received wrong packet type"
                        //", expecting player index");
                
            //}
            //packet >> player_index;
            //local_player = player_index;
        //}

        //void step(ash::Game_state& state,
                //const ash::Environment::Action& a) {
            //using ::operator>>;
            //using ::operator<<;
            //sf::Packet input;
            //input << Packet_type::input_report << a;
            //send_packet(input);
            //auto response = receive_packet();
            //sf::Int32 packet_type;
            //response >> packet_type;
            //if (packet_type != Packet_type::state_report) {
                //throw_error("Expecting state report");
            //}
            //response >> state;
        //}

        //int get_local_player() {
            //return local_player;
        //}

    //private:
        //int local_player;
//};

//void ash::Game_loop::disable_player(size_t player) {
    //players[player].reset(new NullPlayer(player));
//}

//void ash::Game_loop::set_local_player(size_t player) {
    //players[player].reset(new LocalPlayer(player, *window, game_view));
//}

//void ash::Game_loop::set_remote_client(size_t player, unsigned short port) {
    //std::cout << "Waiting for client..." << std::endl;
    //players[player].reset(new RemoteClient(player, port));
    //std::cout << "Client connected" << std::endl;
//}

//void ash::Game_loop::set_remote_server(const std::string& address, unsigned short port) {
    //std::cout << "Connecting to server..." << std::endl;
    //remote_server.reset(new Remote_server(address, port));
    //std::cout << "Connected to server" << std::endl;
    //set_local_player(remote_server->get_local_player());
//}

ash::Game_loop::Game_loop() :
    zoom(1)
{
    window.reset(new sf::RenderWindow(
                sf::VideoMode(600,480), "Airhockey"));
    window->setVerticalSyncEnabled(true);
    reset_view();
    ui_font.loadFromFile("Spaceport_2006.otf");
}

void ash::Game_loop::run() {
    setup();
    while (window->isOpen()) {
        process_events();
        update();
        render(true);
    }
}

ash::Vector_2d ash::Game_loop::get_mouse_coords() const {
    auto mouse_pixel = sf::Mouse::getPosition(*window);
    auto[mouse_x, mouse_y] = window->mapPixelToCoords(
            mouse_pixel, game_view);
    return ash::Vector_2d(mouse_x, -mouse_y);
}

void ash::Game_loop::set_mouse_position(const ash::Vector_2d& coords) {
    sf::Vector2f coords_2f(coords.x, coords.y);
    auto pixel = window->mapCoordsToPixel(coords_2f, game_view);
    sf::Mouse::setPosition(pixel, *window);
}

void ash::Game_loop::setup() {

}

void ash::Game_loop::update() {

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
                zoom = std::max(0.05, zoom + delta*0.05);
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
    auto position = body.get_position();
    if (extrapolate) {
        position += body.get_velocity()*game_state.accumulator;
    }
    double x = position.x;
    double y = -position.y;
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

void ash::Local_player::report_state(const Game_state& state) {
    if (state.new_game) {
        int player = get_index();
        auto mallet_position = state.environment
            .get_mallets()[player].get_position();
        game_loop->set_mouse_position(mallet_position);
    }
}


ash::Vector_2d ash::Local_player::acquire_input() {
    return game_loop->get_mouse_coords();
}

ash::Remote_player::Remote_player(int index, sf::TcpListener& listener) :
    Player(index)
{
    auto status = listener.accept(client);
    if (status != sf::Socket::Done) {
        throw Network_error("Error trying to accept new connection");
    }
    client.setBlocking(false);
    sf::Packet packet;
    packet << Packet_type::player_index << sf::Int32(get_index());
    send_packet(client, packet, sf::milliseconds(10));
}

void ash::Remote_player::report_state(const Game_state& state) {
    using ::operator<<;
    sf::Packet packet;
    packet << Packet_type::state_report << state; 
    send_packet(client, packet, sf::milliseconds(10));
}

ash::Vector_2d ash::Remote_player::acquire_input() {
    ash::Vector_2d input;
    auto packet = receive_packet(client, sf::milliseconds(100));
    Packet_type packet_type;
    packet >> packet_type;
    if (packet_type != Packet_type::input_report) {
        throw Network_error("Expecting input report");
    }
    packet >> input;
    return input;
}

ash::Server_loop::Server_loop(int local_player, unsigned short port) :
    local_player(local_player), port(port)
{

}

void ash::Server_loop::setup() {
    sf::TcpListener listener;
    auto status = listener.listen(port);
    if (status != sf::Socket::Done) {
        throw Network_error("Couldn't bind listener to " +
                std::to_string(port));
    }
    if (local_player < 0) {
        // dedicated server
        for (int i = 0; i < 2; ++i) {
            std::cout << "Waiting for player " << i << "..." << std::endl;
            players[i].reset(new Remote_player(i, listener));
            std::cout << "Player " << i << " connected" << std::endl;
        }
    }
    else {
        int remote = 1 - local_player;
        players[local_player].reset(new Local_player(local_player, this));
        std::cout << "Waiting for player " << remote << "..." << std::endl;
        players[remote].reset(
                new Remote_player(remote, listener));
        std::cout << "Player " << remote << " connected" << std::endl;
    }
    start_new_game();
    report_to_players();
}

void ash::Server_loop::update() {
    game_state.accumulator += clk.restart().asSeconds();
    while (game_state.accumulator > parameters::dt) {
        auto input1 = players[0]->acquire_input();
        auto input2 = players[1]->acquire_input();
        int winner = game_state.environment.step(input1, input2);
        if (winner != -1) {
            ++game_state.score[winner];
            start_new_game(1 - game_state.sender);
        }
        else {
            game_state.new_game = false;
            game_state.accumulator -= parameters::dt;
        }
        report_to_players();
    }
}

void ash::Server_loop::start_new_game(int sender) {
    game_state.sender = sender;
    game_state.environment.reset(sender);
    game_state.new_game = true;
    game_state.accumulator = 0;
    clk.restart();
}

void ash::Server_loop::report_to_players() {
    for (int i = 0; i < 2; ++i) {
        players[i]->report_state(game_state);
    }
}

ash::Client_loop::Client_loop(const std::string& address, unsigned short port) :
    address(address), port(port)
{

            //using ::operator>>;
            //status = remote.connect(address, port);
            //if (status != sf::Socket::Done) {
                //throw_error();
            //}
            //remote.setBlocking(false);
            //auto packet = receive_packet(100e-3);
            //sf::Int32 packet_type, player_index;
            //packet >> packet_type;
            //if (packet_type != Packet_type::player_index) {
                //throw_error("Received wrong packet type"
                        //", expecting player index");
                
            //}
            //packet >> player_index;
            //local_player = player_index;
}

void ash::Client_loop::setup() {
    std::cout << "Trying to connect to server" << std::endl;
    auto status = server.connect(address, port);
    if (status != sf::Socket::Done) {
        throw Network_error("Couldn't connect to " + address + ":" + std::to_string(port));
    }
    std::cout << "Connected to server" << std::endl;
    server.setBlocking(false);
    auto packet = receive_packet(server, sf::milliseconds(100));
    Packet_type packet_type;
    packet >> packet_type;
    if (packet_type!= Packet_type::player_index) {
        throw Network_error("Have not received local player index");
    }
    sf::Int32 local_player_index;
    packet >> local_player_index;
    local_player.reset(new Local_player(local_player_index, this));
    receive_state();
    clk.restart();
}

void ash::Client_loop::update() {
    game_state.accumulator += clk.restart().asSeconds();
    while (game_state.accumulator > parameters::dt) {
        send_input();
        receive_state();
        local_player->report_state(game_state);
    }
}

void ash::Client_loop::receive_state() {
    auto packet = receive_packet(server, sf::milliseconds(100));
    Packet_type packet_type;
    packet >> packet_type;
    if (packet_type != Packet_type::state_report) {
        throw Network_error("Have not received state report");
    }
    packet >> game_state;
}

void ash::Client_loop::send_input() {
    using ::operator<<;
    sf::Packet packet;
    packet << Packet_type::input_report << local_player->acquire_input();
    send_packet(server, packet, sf::milliseconds(10));
}
