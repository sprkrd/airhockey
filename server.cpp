#include <SFML/Network.hpp>
#include <cassert>
#include <iostream>
#include "player.hpp"

int main() {
    /*
    sf::UdpSocket socket;
    unsigned short recv_port = 54000;
    unsigned short send_port = 54001;
    unsigned short ign_p;
    sf::IpAddress ign_a;
    sf::IpAddress recipient = "127.0.0.1";
    if (socket.bind(recv_port) != sf::Socket::Done) {
        std::cerr << "Couldn't bind to port " << recv_port << '\n';
        exit(-1);
    }

    sf::Packet packet;
    sf::Packet response;
    packet << sf::Int32(42);

    sf::Clock clk;

    if (socket.send(packet, recipient, send_port) != sf::Socket::Done) {
        std::cerr << "Couldn't send packet to port " << send_port << '\n';
        exit(-1);
    }

    if (socket.receive(response, ign_a, ign_p) != sf::Socket::Done) {
        std::cerr << "Couldn't receive packet from port "
                  << recv_port << '\n';
        exit(-1);
    }

    double elapsed_ms = clk.getElapsedTime().asSeconds()*1000;
    std::cout << elapsed_ms << std::endl;

    sf::Int32 msg;
    response >> msg;

    assert(msg == -1);

    */

    

    sf::TcpListener listener;
    auto status = listener.listen(54000);
    if (status != sf::Socket::Done) {
        std::cerr << "Couldn't bind to port 54000\n";
        exit(-1);
    }
    sf::TcpSocket client;
    status = listener.accept(client);
    if (status != sf::Socket::Done) {
        std::cerr << "Couldn't accept TCP connection\n";
        exit(-1);
    }



    sf::Packet packet;
    packet << sf::Int32(42);

    sf::Clock clk;

    status = client.send(packet);
    
    if (status != sf::Socket::Done) {
        std::cerr << "Couldn't send packet\n";
        exit(-1);
    }

    status = client.receive(packet);
    double elapsed = clk.getElapsedTime().asSeconds()*1000;
    if (status != sf::Socket::Done) {
        std::cerr << "Couldn't receive packet\n";
        exit(-1);
    }
    std::cout << elapsed << " ms" << std::endl;

    sf::Int32 msg;
    packet >> msg;
    assert(msg==-1);
    std::cout << msg << std::endl;



}
