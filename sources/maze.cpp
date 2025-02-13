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
, block_type(1) {
    
}

maze::maze(std::unique_ptr<grid_interface>&& g)
: my_grid(std::move(g)) {

}

optional<tuple<int, int, int, int>> maze::maze::find_block(int p, int q) const noexcept {
    auto itr = m_p_q.find({ p, q });
    return (itr != m_p_q.cend()) ? make_optional(itr->second) : nullopt;
}

void maze::intopq(int x, int y, int z, int w) noexcept {
    m_p_q[{x, z}] = make_tuple(x, y, z, w);
}
