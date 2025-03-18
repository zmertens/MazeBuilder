#ifndef MAZE_H
#define MAZE_H

#include <string>
#include <memory>

namespace mazes {

class grid_interface;

/// @file maze.h
/// @class maze
/// @brief Data class representing a 2D or 3D maze
class maze {
public:
    explicit maze(std::unique_ptr<grid_interface> g) noexcept;

    int get_levels() const noexcept;

    int get_block_id() const noexcept;

    bool has_distances() const noexcept;

    void set_levels(int levels) noexcept;
    void set_block_id(int block_id) noexcept;
    void set_distances(bool distances) noexcept;

    const std::unique_ptr<grid_interface>& get_grid() const noexcept;

private:
    std::unique_ptr<grid_interface> m_grid;

    int levels;
    bool distances;
    int block_id;
}; // maze struct

} // namespace mazes

#endif // MAZE_H
