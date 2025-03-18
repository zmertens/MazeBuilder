#include <MazeBuilder/maze.h>

#include <MazeBuilder/grid_interface.h>

using namespace mazes;

maze::maze(std::unique_ptr<grid_interface> g) noexcept
    : m_grid(std::move_if_noexcept(g))
    , levels(1)
    , distances(false)
    , block_id(0) {
}

int maze::get_levels() const noexcept {
    return this->levels;
}

int maze::get_block_id() const noexcept {
    return this->block_id;
}

bool maze::has_distances() const noexcept {
    return this->distances;
}

void maze::set_levels(int levels) noexcept {
    this->levels = levels;
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
