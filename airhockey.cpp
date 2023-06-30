#include "physics.hpp"
#include "game_loop.hpp"

#include <SFML/Graphics.hpp>

#include <iostream>


int main(int args, char* argv[]) {
    ash::Game_loop* game_loop = new ash::Server_loop(1, 18000);
    game_loop->run();
}

