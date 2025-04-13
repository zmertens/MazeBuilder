#ifndef SNAKE_HPP
#define SNAKE_HPP

#include <string>
#include <memory>

#include <MazeBuilder/singleton_base.h>

class Physics : public mazes::singleton_base<Physics> {
    friend class mazes::singleton_base<Physics>;
public:
    Physics(const std::string& title, const std::string& version, int w, int h);
    ~Physics();

    bool run() const noexcept;
private:
    struct PhysicsImpl;
    std::unique_ptr<PhysicsImpl> m_impl;
};

#endif
