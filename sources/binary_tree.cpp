#include <MazeBuilder/binary_tree.h>

#include <functional>
#include <memory>
#include <random>
#include <stack>
#include <vector>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>

/// @brief Generate maze in the direction of NORTH and EAST, starting in bottom - left corner of 2D grid
/// @param g
/// @param rng
/// @return 
bool mazes::binary_tree::run(grid_interface* g, randomizer& rng) const noexcept {
    using namespace std;

    if (auto gg = dynamic_cast<grid*>(g)) {

        // Process each node from the map
        for (const auto& [index, c] : gg->m_cells) {
            //auto c = node_ptr->cell_ptr;
            if (!c) continue;

            std::vector<std::shared_ptr<cell>> neighbors;

            auto north_cell = gg->get_north(cref(c));
            if (north_cell) {

                neighbors.emplace_back(north_cell);
            }

            auto east_cell = gg->get_east(cref(c));
            if (east_cell) {

                neighbors.emplace_back(east_cell);
            }

            // Skip linking stage if we have no neighbor, prevent RNG out-of-bounds
            if (!neighbors.empty()) {

                auto random_index = static_cast<int>(rng(0, static_cast<int>(neighbors.size()) - 1));
                if (auto neighbor = neighbors.at(random_index)) {

                    mazes::lab::link(c, neighbor, true);
                }
            }
        }

        return true;
    }

    return false;
}
