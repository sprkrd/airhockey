#include "viz.hpp"

#include <SFML/Network.hpp>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace ash {


class Player {
    public:
        typedef std::unique_ptr<Player> Ptr;
        typedef std::unique_ptr<const Player> ConstPtr;

        virtual Environment::Action get_next_action() = 0;

        virtual void report_state(const Environment::State& state) = 0;

        virtual ~Player() = default;
};

class Local_player : public Player {
    public:
        Local_player(const Renderer& renderer);

        Environment::Action get_next_action() override;

        void report_state(const Environment::State& state) override;

    private:
        const Renderer& renderer;

};

template<class T>
class Sync_queue {
    public:
        void push(T&& t) {
            {
                std::unique_lock lck{mtx};
                q.push(std::move(t));
            }
            cv.notify_one();
        }

        std::optional<T> pop(bool block = false) {
            std::unique_lock lck{mtx};
            if (block) {
                cv.wait(lck, [this]{return !q.empty();});
            }
            if (!q.empty()) {
                auto ret = std::move(q.front());
                q.pop();
                return ret;
            }
            return {};
        }

        std::optional<T> pop(double timeout_s) {
            auto timeout = std::chrono::duration<double>(timeout_s);
            std::unique_lock lck{mtx};
            bool ok = cv.wait_for(lck, timeout,
                    [this]{return !q.empty();});
            if (ok) {
                auto ret = std::move(q.front());
                q.pop();
                return ret;
            }
            return {};
        }

    private:
        std::mutex mtx;
        std::condition_variable cv;
        std::queue<T> q;
};

sf::Packet& operator<<(sf::Packet& packet, const Vector_2d& v);

sf::Packet& operator>>(sf::Packet& packet, Vector_2d& v);

sf::Packet& operator<<(
        sf::Packet& packet,
        const Environment::State::BodyStatus& status);


sf::Packet& operator>>(
        sf::Packet& packet,
        Environment::State::BodyStatus& status);

sf::Packet& operator<<(
        sf::Packet& packet,
        const ash::Environment::State& state);

sf::Packet& operator>>(
        sf::Packet& packet,
        ash::Environment::State& state);

class Remote_player : public Player {
    public:
        Remote_player(double max_latency = 100e-3);

        Environment::Action get_next_action() override;

        void report_state(const Environment::State& state) override;

        void stop() {
            alive = false;
        }

        ~Remote_player() override {
            stop();
            thread.join();
        }

    private:

        sf::Socket::Status receive(sf::Packet& packet, double timeout);

        void run();

        std::thread thread;
        double max_latency;
        Sync_queue<sf::Packet> out_queue;
        Sync_queue<sf::Packet> in_queue;
        sf::TcpListener listener;
        sf::TcpSocket client;
        volatile bool alive;

};

}
