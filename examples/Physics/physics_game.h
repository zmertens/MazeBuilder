#ifndef PHYSICS_GAME_HPP
#define PHYSICS_GAME_HPP

#include <memory>
#include <string>
#include <string_view>

#include <MazeBuilder/algo_interface.h>
#include <MazeBuilder/singleton_base.h>

namespace mazes
{
    class grid_interface;
    class randomizer;
}

class physics_game : public mazes::algo_interface, public mazes::singleton_base<physics_game>
{
    friend class singleton_base;

public:
    physics_game(std::string_view title, std::string_view version, int w, int h);

    physics_game(const std::string& title, const std::string& version, int w, int h);

    ~physics_game() override;

    bool run(mazes::grid_interface* g, mazes::randomizer& rng) const noexcept override;

private:
    struct physics_game_impl;
    std::unique_ptr<physics_game_impl> m_impl;
};

#endif // PHYSICS_GAME_HPP
