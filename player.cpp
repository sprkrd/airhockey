#include "player.hpp"

#include <cassert>
#include <stdexcept>
#include <iostream>

ash::Local_player::Local_player(const Renderer& renderer) : 
    renderer(renderer) {

}

ash::Environment::Action ash::Local_player::get_next_action() {
    return renderer.get_mouse_in_world();
}

void ash::Local_player::report_state(const Environment::State& state) {
    // do nothing
}

sf::Packet& ash::operator<<(sf::Packet& packet, const ash::Vector_2d& v) {
    return packet << v.x << v.y;
}

sf::Packet& ash::operator>>(sf::Packet& packet, ash::Vector_2d& v) {
    return packet >> v.x >> v.y;
}

sf::Packet& ash::operator<<(
        sf::Packet& packet,
        const ash::Environment::State::BodyStatus& status) {
    return packet << status.position << status.velocity;
}

sf::Packet& ash::operator>>(
        sf::Packet& packet,
        ash::Environment::State::BodyStatus& status) {
    return packet >> status.position >> status.velocity;
}

sf::Packet& ash::operator<<(
        sf::Packet& packet,
        const ash::Environment::State& state) {
    return packet << state.mallets[0] << state.mallets[1]
                  << state.puck;
}

sf::Packet& ash::operator>>(
        sf::Packet& packet,
        ash::Environment::State& state) {
    return packet >> state.mallets[0] >> state.mallets[1]
                  >> state.puck;
}

ash::Remote_player::Remote_player(double max_latency) :
    max_latency(max_latency), alive(true)
{
    assert(listener.listen(54000) == sf::Socket::Done);

    assert(listener.accept(client) == sf::Socket::Done);

    thread = std::thread(&Remote_player::run, this);
}

ash::Environment::Action ash::Remote_player::get_next_action() {
    auto packet = in_queue.pop(max_latency);
    if (packet) {
        Environment::Action action;
        *packet >> action;
        return action;
    }
    throw std::runtime_error("max time exceeded");
}

void ash::Remote_player::report_state(const Environment::State& state) {
    sf::Packet packet;
    packet << state;
    out_queue.push(std::move(packet));
}

sf::Socket::Status ash::Remote_player::receive(sf::Packet& packet,
        double timeout) {
    sf::SocketSelector selector;
    selector.add(client);
    if (selector.wait(sf::seconds(timeout))) {
        return client.receive(packet);
    }
    else {
        return sf::Socket::NotReady;
    }
}

void ash::Remote_player::run() {
    using namespace ash::parameters;
    while (alive) {
        auto outgoing_msg = out_queue.pop(true);
        sf::Packet response;
        client.send(*outgoing_msg);
        auto status = receive(response, dt + max_latency);
        if (status != sf::Socket::Done) {
            std::cerr << "Didn't receive packet on time, shutting down...\n";
            alive = false;
        }
        else {
            in_queue.push(std::move(response));
        }
    }
}

