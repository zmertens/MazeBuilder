#include <MazeBuilder/maze.h>

#include <algorithm>
#include <functional>

#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_interface.h>

using namespace mazes;
using namespace std;

/// @brief 
/// @param rows 
/// @param columns 
/// @param height 1
maze::maze(unsigned int rows, unsigned int columns, unsigned int height) 
: my_grid(make_unique<grid>(rows, columns, height))
, distances(false)
, block_id(1) {
    
}

maze::maze(std::unique_ptr<grid_interface>&& g)
: my_grid(std::move(g)) {

}

unsigned int maze::get_rows() const noexcept {
    return std::get<0>(my_grid->get_dimensions());
}

unsigned int maze::get_columns() const noexcept {
    return std::get<1>(my_grid->get_dimensions());
}

unsigned int maze::get_levels() const noexcept {
    return std::get<2>(my_grid->get_dimensions());
}

bool maze::has_distances() const noexcept {
    return distances;
}

int maze::get_block_id() const noexcept {
    return block_id;
}

optional<tuple<int, int, int, int>> maze::maze::find_block(int p, int q) const noexcept {
    auto itr = m_p_q.find({ p, q });
    return (itr != m_p_q.cend()) ? make_optional(itr->second) : nullopt;
}

void maze::intopq(int x, int y, int z, int w) noexcept {
    m_p_q[{x, z}] = make_tuple(x, y, z, w);
}

const std::unique_ptr<grid_interface>& maze::get_grid() const noexcept {
    return my_grid;
}
