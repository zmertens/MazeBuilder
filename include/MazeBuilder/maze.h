#ifndef MAZE_H
#define MAZE_H

#include <string>
#include <memory>

namespace mazes {

class configurator;
class grid_interface;

/// @file maze.h
/// @class maze
/// @brief Data class representing a 2D or 3D maze
class maze {
public:
    explicit maze(std::unique_ptr<grid_interface> g, configurator const& config) noexcept;

    int get_block_id() const noexcept;

    bool has_distances() const noexcept;

    int get_rows() const noexcept;
    int get_columns() const noexcept;
    int get_levels() const noexcept;

    const std::unique_ptr<grid_interface>& get_grid() const noexcept;

private:
    std::unique_ptr<grid_interface> m_grid;

    const configurator& m_config;
}; // maze struct

} // namespace mazes

#endif // MAZE_H
