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
bool binary_tree::run(std::unique_ptr<grid_interface> const& g, randomizer& rng) const noexcept {

    using namespace std;

    const auto& grid_ops = g->operations();

    // Process each node from the map
    for (const auto& c : grid_ops.get_cells()) {
        
        if (!c) {

            continue;
        }

        std::vector<std::shared_ptr<cell>> neighbors;

        auto north_cell = grid_ops.get_north(cref(c));
        
        if (north_cell) {

            neighbors.emplace_back(north_cell);
        }

        auto east_cell = grid_ops.get_east(cref(c));
        if (east_cell) {

            neighbors.emplace_back(east_cell);
        }

        // Skip linking stage if we have no neighbor, prevent RNG out-of-bounds
        if (!neighbors.empty()) {

            auto random_index = static_cast<int>(rng(0, static_cast<int>(neighbors.size()) - 1));

            if (auto neighbor = neighbors.at(random_index)) {

                lab::link(c, neighbor, true);
            }
        }
    }

    return true;
}

