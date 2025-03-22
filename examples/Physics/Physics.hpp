#ifndef SNAKE_HPP
#define SNAKE_HPP

#include <string>
#include <memory>

class Snake {
public:
    Snake(const std::string& title, const std::string& version, int w, int h);
    ~Snake();

    // Delete copy constructor and copy assignment operator
    Snake(const Snake&) = delete;
    Snake& operator=(const Snake&) = delete;

    // Default move constructor and move assignment operator
    Snake(Snake&&) = default;
    Snake& operator=(Snake&&) = default;

    bool run() const noexcept;

    // Singleton pattern
    static std::shared_ptr<Snake> get_instance(const std::string& title, const std::string& version, int w, int h) {
        static std::shared_ptr<Snake> instance = std::make_shared<Snake>(cref(title), std::cref(version), w, h);
        return instance;
    }
private:
    struct SnakeImpl;
    std::unique_ptr<SnakeImpl> m_impl;
};

#endif
