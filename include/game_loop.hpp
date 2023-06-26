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
        typedef std::unique_ptr<PlayerImpl> PlayerImplPtr;

        Game_loop();

        void disable_player(size_t player);

        void set_local_player(size_t player);

        void set_remote_client(size_t player, unsigned short port);

        void set_remote_server(const std::string& address,
                unsigned short port);

        void run();

        ~Game_loop();

    private:
        void start_new_game(int sender);

        void shutdown();

        void reset_view();

        void process_events();

        void update_game_state();

        void report_state_to_players();

        void render();

        void draw_score();

        void draw_body(const Body& body, const sf::Color& color);

        std::unique_ptr<sf::RenderWindow> window;
        sf::View game_view;
        sf::View ui_view;
        sf::Font ui_font;
        double zoom;

        Game_state game_state;
        sf::Clock clk;
        std::array<PlayerImplPtr,2> players;
};

}
