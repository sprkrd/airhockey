#include <SFML/Graphics.hpp>
#include "physics.hpp"


namespace ash {

class Game_loop {
    public:
        Game_loop();

        void process_events();

        void render();

    private:

        sf::RenderWindow window;
        Environment env;


}
