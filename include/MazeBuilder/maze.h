#ifndef MAZE_H
#define MAZE_H

#include <string>
#include <optional>

#include <MazeBuilder/hash_funcs.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid_interface.h>

namespace mazes {

class cell;

/// @file maze.h
/// @class maze
/// @brief Data class representing a 2D or 3D maze
class maze {
public:

    using pqmap = std::unordered_map<std::pair<int, int>, std::tuple<int, int, int, int>, hash_funcs>;

    explicit maze(unsigned int rows, unsigned int columns, unsigned int levels = 1);
    explicit maze(std::unique_ptr<grid_interface>&& g);

    // Getters
    unsigned int get_rows() const noexcept;
    unsigned int get_columns() const noexcept;
    unsigned int get_levels() const noexcept;
    bool has_distances() const noexcept;
    int get_block_id() const noexcept;

    std::optional<std::tuple<int, int, int, int>> find_block(int x, int z) const noexcept;

    void intopq(int x, int y, int z, int w) noexcept;

    const std::unique_ptr<grid_interface>& get_grid() const noexcept;
private:
    bool distances;
    int block_id;

    pqmap m_p_q;

    std::unique_ptr<grid_interface> my_grid;
}; // maze struct

} // namespace mazes

#endif // MAZE_H
