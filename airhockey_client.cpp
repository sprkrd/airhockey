#include "physics.hpp"
#include "game_loop.hpp"
#include <iostream>


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " address\n";
        return 1;
    }
    std::string address(argv[1]);

    std::unique_ptr<ash::Game_loop> game_loop(new ash::Client_loop(address, 18000));
    try {
        game_loop->run();
    } catch (ash::Network_error& e) {
        std::cout << e.what() << '\n'
                  << "Disconnected" << std::endl;
    }
}

