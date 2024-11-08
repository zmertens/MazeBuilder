#ifndef GAME_HPP
#define GAME_HPP

#include <string>
#include <memory>

class Game {
public:
    Game(const std::string& title, const std::string& version, int w, int h);
    ~Game();

    // Delete copy constructor and copy assignment operator
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // Default move constructor and move assignment operator
    Game(Game&&) = default;
    Game& operator=(Game&&) = default;

    bool run(const std::string& workerUrl, const std::string& lastSaveFile) const noexcept;

    // Singleton pattern
    static std::shared_ptr<Game> get_instance(const std::string& title, const std::string& version, int w, int h) {
        static std::shared_ptr<Game> instance = std::make_shared<Game>(cref(title), std::cref(version), w, h);
        return instance;
    }
private:
    struct GameImpl;
    std::unique_ptr<GameImpl> m_impl;
};

#endif
