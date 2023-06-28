#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include "physics.hpp"

namespace ash {

struct Game_state {
    Environment environment;
    std::array<int,2> score;
    double accumulator;
    int sender;
    bool new_game;
};

class Game_loop {
    public:
        class PlayerImpl;
        class Remote_server;
        typedef std::unique_ptr<PlayerImpl> PlayerImplPtr;
        typedef std::unique_ptr<Remote_server> Remote_server_ptr;

        Game_loop();

        void run();

        virtual ~Game_loop();

    protected:

        virtual void setup();

        virtual void update();

        virtual void shutdown();

    private:

        void reset_view();

        void process_events();

        void render(bool extrapolate = true);

        void draw_score();

        void draw_body(const Body& body, const sf::Color& color,
                bool extrapolate);

        std::unique_ptr<sf::RenderWindow> window;
        sf::View game_view;
        sf::View ui_view;
        sf::Font ui_font;
        double zoom;

        Game_state game_state;
};


class Server_loop : public Game_loop {
};

class Client_loop : public Game_loop {
};

}
