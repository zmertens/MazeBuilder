#ifndef CRAFT_H
#define CRAFT_H

#include <string>
#include <string_view>
#include <memory>
#include <functional>

#include "maze_types_enum.h"

class craft {
public:
    craft(const std::string_view& window_name, const std::string_view& version, const std::string_view& help);
    ~craft();

    // Delete copy constructor and copy assignment operator
    craft(const craft&) = delete;
    craft& operator=(const craft&) = delete;

    // Default move constructor and move assignment operator
    craft(craft&&) = default;
    craft& operator=(craft&&) = default;

    bool run(const std::function<int(int, int)>& get_int, const std::function<mazes::maze_types(const std::string& algo)> get_maze_algo_from_str) const noexcept;
    
    std::string get_vertex_data() const noexcept;

private:
    struct craft_impl;
    std::unique_ptr<craft_impl> m_pimpl;

};

#endif // CRAFT_H
