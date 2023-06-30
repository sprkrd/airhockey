#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include "physics.hpp"

namespace ash {

struct Game_state {
    Environment environment;
    double accumulator;
    std::array<int,2> score;
    int sender;
    bool new_game;
};

class Game_loop {
    public:
        Game_loop();

        void run();

        Vector_2d get_mouse_coords() const;

        void set_mouse_position(const Vector_2d& coords);

        virtual ~Game_loop() = default;

    protected:

        virtual void setup();

        virtual void update();

        virtual void shutdown();

        Game_state game_state;

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
};

class Network_error : public std::runtime_error {
    public:

        Network_error(const std::string& what_arg) :
            runtime_error(what_arg)
        {
        }
};


class Player {
    public:

        typedef std::unique_ptr<Player> Ptr;

        Player(int index) : index(index) {
        }

        int get_index() const {
            return index;
        }

        virtual void report_state(const Game_state& state) = 0;

        virtual Vector_2d acquire_input() = 0;

        virtual ~Player() = default;

    private:

        int index;
};

class Local_player : public Player {
    public:

        Local_player(int index, Game_loop* game_loop) :
            Player(index), game_loop(game_loop)
        {
        }

        void report_state(const Game_state& state) override;

        Vector_2d acquire_input() override;

    private:

        Game_loop* game_loop;
};

class Remote_player : public Player {
    public:

        Remote_player(int index, sf::TcpListener& listener);

        void report_state(const Game_state& state) override;

        Vector_2d acquire_input() override;

    private:
        sf::TcpSocket client;
};


class Server_loop : public Game_loop {
    public:
        Server_loop(int local_player, unsigned short port);

    protected:

        void setup() override;

        void update() override;

    private:

        void start_new_game(int sender = 0);

        void report_to_players();

        sf::Clock clk;
        std::array<Player::Ptr,2> players;
        int local_player;
        unsigned short port;
};

class Client_loop : public Game_loop {
    public:

        Client_loop(const std::string& address, unsigned short port);

    protected:

        void setup() override;

        void update() override;

    private:
        void receive_state();

        void send_input();

        sf::Clock clk;
        std::string address;
        unsigned short port;
        sf::TcpSocket server;
        Player::Ptr local_player;
};

}
