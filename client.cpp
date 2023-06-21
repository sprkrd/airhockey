#include <SFML/Network.hpp>
#include <cassert>
#include <iostream>
#include "player.hpp"

int main() {
    /*
    sf::UdpSocket socket;
    unsigned short recv_port = 54001;
    unsigned short send_port = 54000;
    unsigned short ign_p;
    sf::IpAddress ign_a;
    sf::IpAddress recipient = "127.0.0.1";
    if (socket.bind(recv_port) != sf::Socket::Done) {
        std::cerr << "Couldn't bind to port " << recv_port << '\n';
        exit(-1);
    }

    sf::Packet packet;
    sf::Packet response;
    packet << sf::Int32(-1);

    if (socket.receive(response, ign_a, ign_p) != sf::Socket::Done) {
        std::cerr << "Couldn't receive packet from port "
                  << recv_port << '\n';
        exit(-1);
    }

    if (socket.send(packet, recipient, send_port) != sf::Socket::Done) {
        std::cerr << "Couldn't send packet to port " << send_port << '\n';
        exit(-1);
    }

    sf::Int32 msg;
    response >> msg;

    std::cout << msg << std::endl;

    assert(msg == 42);

    */

    
    sf::TcpSocket server;
    auto status = server.connect("localhost", 54000);
    if (status != sf::Socket::Done) {
        std::cerr << "Couldn't connect to server\n";
        exit(-1);
    }

    sf::Packet packet;

    status = server.receive(packet);
    if (status != sf::Socket::Done) {
        std::cerr << "Couldn't receive packet\n";
        exit(-1);
    }
    sf::Int32 msg;
    packet >> msg;
    assert(msg == 42);
    std::cout << msg << std::endl;
    packet = sf::Packet();
    packet << sf::Int32(-1);
    status = server.send(packet);
    if (status != sf::Socket::Done) {
        std::cerr << "Couldn't send packet\n";
        exit(-1);
    }
    
}
