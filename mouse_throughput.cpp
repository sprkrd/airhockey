#include <SFML/Graphics.hpp>
#include <iostream>
#include <thread>
#include <mutex>
#include <X11/Xlib.h>


double x = 0;
double y = 0;

std::mutex env_mtx, x11_mtx;


class Rate {
    public:

        typedef std::chrono::steady_clock Clock;
        typedef typename Clock::time_point Time_point;
        typedef typename Clock::duration Duration;

        Rate(double frequency) :
            start(Clock::now()),
            expected_cycle(
                    std::chrono::duration_cast<Duration>(
                        std::chrono::duration<double>(1.0/frequency)))
        {
        }

        void delay() {
            auto expected_end = start + expected_cycle;
            auto current_time = Clock::now();
            auto sleep_time = expected_end - current_time;
            actual_cycle = current_time - start;
            start = expected_end;
            if (sleep_time <= Duration(0)) {
                if (current_time >= expected_end + expected_cycle) {
                    start = current_time;
                }
                return;
            }
            //while (Clock::now() < expected_end);
            std::this_thread::sleep_for(sleep_time);
        }

        double get_actual_cycle() const {
            return 1000*std::chrono::duration<double>(actual_cycle).count();
        }

    private:
        Time_point start;
        Duration expected_cycle;
        Duration actual_cycle;
};

void sim(const sf::RenderWindow& win) {
    {
        // center cursor
        std::unique_lock lck{x11_mtx};
        sf::Mouse::setPosition(
                win.mapCoordsToPixel(sf::Vector2f(0,0)), win);
    }
    size_t report_count = 10;
    Rate rate(10);
    size_t update_count = 0;
    sf::Clock clk;
    while (win.isOpen()) {
        double dx, dy;
        {
            // input handling
            std::unique_lock lck{x11_mtx};
            auto mouse_world_pos = win.mapPixelToCoords(
                    sf::Mouse::getPosition(win));
            dx = mouse_world_pos.x;
            dy = mouse_world_pos.y;
            sf::Mouse::setPosition(
                    win.mapCoordsToPixel(sf::Vector2f(0,0)), win);
        }
        {
            // update
            std::unique_lock lck{env_mtx};
            x += dx;
            y += dy;
        }
        rate.delay();
        if (++update_count == report_count) {
            double avg_rate = report_count/clk.restart().asSeconds();
            update_count = 0;
            std::cout << avg_rate << " hz" << std::endl;
            std::cout << "Actual cycle: " << rate.get_actual_cycle() << " ms" << std::endl;
        }
    }
}


int main() {
    sf::RenderWindow win(sf::VideoMode(600,480), "Test");
    win.setVerticalSyncEnabled(true);
    win.setView(sf::View(sf::Vector2f(0,0), sf::Vector2f(600,480)));
    std::thread sim_thread(&sim, std::ref(win));
    while (win.isOpen()) {
        {
            std::unique_lock lck{x11_mtx};
            sf::Event event;
            while (win.pollEvent(event)) {
                switch (event.type) {
                    case sf::Event::Closed:
                        win.close();
                        break;
                    default:
                        break;
                }
            }
        }

        win.clear(sf::Color::White);
        {
            std::unique_lock lck{env_mtx};
            // scene drawing
            auto circle = sf::CircleShape(100);
            circle.setOrigin(100,100);
            circle.setPosition(x,y);
            circle.setFillColor(sf::Color::Red);
            win.draw(circle);
        }
        {
            std::unique_lock lck{x11_mtx};
            win.display();
        }
    }
    sim_thread.join();
}
