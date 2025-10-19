#ifndef PHYSICS_HPP
#define PHYSICS_HPP

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

#include <MazeBuilder/singleton_base.h>

struct SDL_Renderer;

class Physics : public mazes::singleton_base<Physics> {
    friend class mazes::singleton_base<Physics>;
public:
    Physics(const std::string& title, const std::string& version, int w, int h);
    ~Physics();

    bool run() const noexcept;

private:
    // Physics and collision processing
    void processPhysicsCollisions() const;
    void updatePhysicsObjects() const;
       
    // Rendering methods
    void drawPhysicsObjects(SDL_Renderer* renderer) const;
    void drawMaze(SDL_Renderer* renderer, const std::string_view& cells, int display_w, int display_h) const;
    void generateNewLevel(std::string& persistentMazeStr, int display_w, int display_h) const;
    void generateMazeWithDistances(std::string& persistentMazeStr, int display_w, int display_h) const;

    struct PhysicsImpl;
    std::unique_ptr<PhysicsImpl> m_impl;
};

#endif // PHYSICS_HPP
