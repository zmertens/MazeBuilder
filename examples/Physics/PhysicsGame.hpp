#ifndef PHYSICS_GAME_HPP
#define PHYSICS_GAME_HPP

#include <memory>
#include <string>
#include <string_view>

#include <MazeBuilder/singleton_base.h>

struct SDL_Renderer;

class PhysicsGame : public mazes::singleton_base<PhysicsGame> {
    friend class mazes::singleton_base<PhysicsGame>;
public:
    PhysicsGame(const std::string& title, const std::string& version, int w, int h);

    ~PhysicsGame();

    bool run() const noexcept;

private:

    struct PhysicsGameImpl;
    std::unique_ptr<PhysicsGameImpl> m_impl;
};

#endif // PHYSICS_GAME_HPP
