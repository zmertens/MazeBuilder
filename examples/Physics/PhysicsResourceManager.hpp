#ifndef PHYSICS_RESOURCE_MANAGER_HPP
#define PHYSICS_RESOURCE_MANAGER_HPP

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <MazeBuilder/singleton_base.h>

class Texture;

class PhysicsResourceManager : public mazes::singleton_base<PhysicsResourceManager> {
    friend class mazes::singleton_base<PhysicsResourceManager>;
public:

    PhysicsResourceManager();
    ~PhysicsResourceManager();
    
    // Complete resource initialization
    struct PhysicsResources {
        std::string splashPath;
        int splashWidth = 0;
        int splashHeight = 0;
        std::string musicPath;
        std::string soundPath;
        std::string windowIconPath;
        bool success = false;
    };

    std::optional<PhysicsResources> initializeAllResources(std::string_view configPath);

private:
    struct PhysicsResourceManagerImpl;
    std::unique_ptr<PhysicsResourceManagerImpl> m_impl;
};

#endif // PHYSICS_RESOURCE_MANAGER_HPP

