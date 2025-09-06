#include <MazeBuilder/binary_tree.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>

#include <functional>
#include <memory>
#include <random>
#include <stack>
#include <vector>

using namespace mazes;

/// @brief Generate maze in the direction of NORTH and EAST, starting in bottom - left corner of a 2D grid
/// @param g
/// @param rng
/// @return
bool binary_tree::run(grid_interface *g, randomizer &rng) const noexcept
{
    using namespace std;

    const auto &grid_ops = g->operations();

    // Use iterator-based approach instead of get_cells() for large grid efficiency
    auto [rows, columns, levels] = grid_ops.get_dimensions();

    // Process each cell position without materializing all cells
    for (auto level{0u}; level < levels; ++level)
    {
        for (auto row{0u}; row < rows; ++row)
        {
            for (auto col{0u}; col < columns; ++col)
            {
                if (auto c = grid_ops.search(static_cast<int>(level * (rows * columns) + row * columns + col)))
                {
                    vector<shared_ptr<cell>> neighbors;

                    if (auto north_cell = grid_ops.get_north(cref(c)))
                    {
                        neighbors.emplace_back(north_cell);
                    }

                    if (auto east_cell = grid_ops.get_east(cref(c)))
                    {
                        neighbors.emplace_back(east_cell);
                    }

                    // Skip linking stage if we have no neighbors, prevent RNG out-of-bounds
                    if (!neighbors.empty())
                    {
                        if (auto neighbor = neighbors.at(static_cast<int>(rng(0, static_cast<int>(neighbors.size()) - 1))))
                        {

                            lab::link(c, neighbor, true);
                        }
                    }
                }
            } // end col loop
        } // end row loop
    } // end level loop

    return true;
}
