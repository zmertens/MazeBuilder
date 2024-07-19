#ifndef CRAFT_H
#define CRAFT_H

#include <string>
#include <memory>
#include <functional>
#include <list>

#include "maze_types_enum.h"

class craft {
public:
    craft(const std::string& window_name, const std::string& version, const std::string& help);
    ~craft();

    // Delete copy constructor and copy assignment operator
    craft(const craft&) = delete;
    craft& operator=(const craft&) = delete;

    // Default move constructor and move assignment operator
    craft(craft&&) = default;
    craft& operator=(craft&&) = default;

    bool run(unsigned long seed, const std::list<std::string>& algos, 
        const std::function<mazes::maze_types(const std::string& algo)> get_maze_algo_from_str) noexcept;
    
    void set_json(const std::string& s) noexcept;
    std::string get_json() const noexcept;
    
    // Singleton pattern
    static std::shared_ptr<craft> get_instance(const std::string& w, const std::string& v, const std::string& h) {
        static std::shared_ptr<craft> instance = std::make_shared<craft>(w, v, h);
        return instance;
    }
private:
    struct craft_impl;
    std::unique_ptr<craft_impl> m_pimpl;
    std::string json;
};

#endif // CRAFT_H
