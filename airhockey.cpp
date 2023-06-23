#include "physics.hpp"
#include "game_loop.hpp"

#include <SFML/Graphics.hpp>

#include <cassert>
#include <iostream>


int main(int args, char* argv[]) {
    ash::Game_loop game_loop;
    game_loop.set_local_player(0);
    game_loop.run();
}

