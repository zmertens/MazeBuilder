#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

#include <MazeBuilder/singleton_base.h>
#include <box2d/box2d.h>

// Forward declarations
namespace mazes {
    class cell;
}

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

#endif // PHYSICS_HPP
