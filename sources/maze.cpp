#include <MazeBuilder/maze.h>

#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/configurator.h>

#include <tuple>

using namespace mazes;

/// @brief 
/// @param g 
/// @param config {}
maze::maze(std::unique_ptr<grid_interface> g, configurator const& config) noexcept
    : m_grid(std::move_if_noexcept(g))
    , m_config(config) {
}

int maze::get_block_id() const noexcept {
    return this->m_config.block_id();
}

bool maze::has_distances() const noexcept {
    return this->m_config.distances();
}

int maze::get_rows() const noexcept {
    return this->m_config.rows();
}

int maze::get_columns() const noexcept {
    return this->m_config.columns();
}

int maze::get_levels() const noexcept {
    return this->m_config.levels();
}

const std::unique_ptr<grid_interface>& maze::get_grid() const noexcept {
    return m_grid;
}
