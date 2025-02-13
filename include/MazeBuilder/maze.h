#ifndef MAZE_H
#define MAZE_H

#include <string>
#include <ostream>
#include <memory>
#include <functional>
#include <random>
#include <unordered_map>
#include <optional>
#include <cstdint>

#include <MazeBuilder/hash.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid_interface.h>

namespace mazes {

class cell;

/// @brief Data class to represent a maze
class maze {
public:

    using dimensions = std::tuple<int, int, int, int>;
    using pqmap = std::unordered_map<std::pair<int, int>, dimensions, pair_hash>;
    using maze_ptr = std::unique_ptr<maze>;

    bool distances;
    int block_type;

    explicit maze(unsigned int rows, unsigned int columns, unsigned int height = 1);
    explicit maze(std::unique_ptr<grid_interface>&& g);

    std::optional<std::tuple<int, int, int, int>> find_block(int x, int z) const noexcept;

    void intopq(int x, int y, int z, int w) noexcept;

    const std::unique_ptr<grid_interface>& get_grid() const noexcept;
private:

    pqmap m_p_q;

    std::unique_ptr<grid_interface> my_grid;
}; // maze struct

} // namespace mazes

#endif // MAZE_H
