#ifndef MAZE_H
#define MAZE_H

#include <string>
#include <memory>
#include <tuple>

namespace mazes {

class configurator;
class grid_interface;

/// @file maze.h
/// @class maze
/// @brief Data class representing a 2D or 3D maze
class maze {
public:
    explicit maze(configurator const& config, std::string_view s) noexcept;

    int get_block_id() const noexcept;

    bool has_distances() const noexcept;

    std::tuple<int, int, int> get_dimensions() const noexcept;

    std::string_view str() const noexcept;
private:
    configurator const& m_config;
    std::string m_str;
}; // maze struct

} // namespace mazes

#endif // MAZE_H
