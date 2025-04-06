#ifndef SNAKE_HPP
#define SNAKE_HPP

#include <string>
#include <memory>

class Physics {
public:
    Physics(const std::string& title, const std::string& version, int w, int h);
    ~Physics();

    // Delete copy constructor and copy assignment operator
    Physics(const Physics&) = delete;
    Physics& operator=(const Physics&) = delete;

    // Default move constructor and move assignment operator
    Physics(Physics&&) = default;
    Physics& operator=(Physics&&) = default;

    bool run() const noexcept;

    // Singleton pattern
    static std::shared_ptr<Physics> get_instance(const std::string& title, const std::string& version, int w, int h) {
        static std::shared_ptr<Physics> instance = std::make_shared<Physics>(cref(title), std::cref(version), w, h);
        return instance;
    }
private:
    struct PhysicsImpl;
    std::unique_ptr<PhysicsImpl> m_impl;
};

#endif
