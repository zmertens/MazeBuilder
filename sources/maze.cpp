#include <MazeBuilder/maze.h>

#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/configurator.h>

#include <tuple>

using namespace mazes;

/// @brief 
/// @param g 
/// @param config {}
maze::maze(configurator const& config, std::string_view s) noexcept
    : m_config(config), m_str{ s } {
}

int maze::get_block_id() const noexcept {
    return this->m_config.block_id();
}

bool maze::has_distances() const noexcept {
    return this->m_config.distances();
}

std::tuple<int, int, int> maze::get_dimensions() const noexcept {
    return std::make_tuple(this->m_config.rows(), this->m_config.columns(), this->m_config.levels());
}

std::string_view maze::str() const noexcept {
    return this->m_str;
}
