#ifndef CRAFT_H
#define CRAFT_H

#include <string>
#include <memory>
#include <functional>
#include <random>

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

    bool run(unsigned long seed, const std::function<mazes::maze_types(const std::string& algo)> get_maze_algo_from_str) const noexcept;
    
    std::string get_vertex_data_as_json() const noexcept;

private:
    struct craft_impl;
    std::unique_ptr<craft_impl> m_pimpl;

};

#endif // CRAFT_H
