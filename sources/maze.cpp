#include <MazeBuilder/maze.h>

#include <MazeBuilder/grid_interface.h>

#include <tuple>

using namespace mazes;

/// @brief 
/// @param g 
/// @param block_id -1
/// @param distances false
maze::maze(std::unique_ptr<grid_interface> g, int block_id, bool distances) noexcept
    : m_grid(std::move_if_noexcept(g))
    , block_id(block_id)
    , distances(distances) {
}

int maze::get_block_id() const noexcept {
    return this->block_id;
}

bool maze::has_distances() const noexcept {
    return this->distances;
}

void maze::set_block_id(int block_id) noexcept {
    this->block_id = block_id;
}

void maze::set_distances(bool distances) noexcept {
    this->distances = distances;
}

const std::unique_ptr<grid_interface>& maze::get_grid() const noexcept {
    return m_grid;
}
