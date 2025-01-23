#ifndef MAZE_H
#define MAZE_H

#include <string>
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

using grid_ptr = std::unique_ptr<grid_interface>;

/// @brief Data class to represent a maze
class maze {
public:
    using dimensions = std::tuple<int, int, int, int>;
    using pqmap = std::unordered_map<std::pair<int, int>, dimensions, pair_hash>;
    using maze_ptr = std::unique_ptr<maze>;

    int rows;
    int columns;
    int height;
    mazes::algos maze_type;
    int seed;
    bool distances;
    int block_type;
    int offset_x, offset_z;
    std::vector<dimensions> vertices;
    std::vector<std::vector<std::uint32_t>> faces;

    explicit maze();

    void init() noexcept;

    std::vector<std::tuple<int, int, int, int>> get_render_vertices() const noexcept;
    std::vector<std::tuple<int, int, int, int>> get_writable_vertices() const noexcept;
    std::vector<std::vector<std::uint32_t>> get_faces() const noexcept;
    std::optional<std::tuple<int, int, int, int>> find_block(int x, int z) const noexcept;
    void populate_cells(std::vector<std::shared_ptr<cell>>& cells) const noexcept;

    std::string to_str() const noexcept;
    std::string to_str64() const noexcept;
    std::string to_json_str(unsigned int pretty_spaces = 4) const noexcept;
    std::string to_json_array_str(const std::vector<maze::maze_ptr>& mazes, unsigned int pretty_spaces) noexcept;
    std::vector<std::uint8_t> to_pixels(const unsigned int cell_size = 3) const noexcept;

    std::string to_wavefront_obj_str64() const noexcept;
    std::string to_wavefront_obj_str() const noexcept;

    void intopq(int x, int y, int z, int w) noexcept;
private:

    pqmap m_p_q;

    grid_ptr my_grid;
}; // maze struct

} // namespace mazes

#endif // MAZE_H
