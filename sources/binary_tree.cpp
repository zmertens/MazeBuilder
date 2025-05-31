#include <MazeBuilder/binary_tree.h>

#include <memory>
#include <vector>
#include <functional>
#include <stack>
#include <random>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/randomizer.h>

/// @brief Generate maze in the direction of NORTH and EAST, starting in bottom - left corner of 2D grid
/// @param g
/// @param rng
/// @return 
bool mazes::binary_tree::run(grid_interface* g, randomizer& rng) const noexcept {
    if (auto gg = dynamic_cast<grid*>(g)) {

        // Process each node from the map
        for (const auto& [index, c] : gg->m_cells) {
            //auto c = node_ptr->cell_ptr;
            if (!c) continue;

            std::vector<std::shared_ptr<cell>> neighbors;
            auto north_cell = c->get_north();
            if (north_cell) {
                neighbors.emplace_back(north_cell);
            }
            auto east_cell = c->get_east();
            if (east_cell) {
                neighbors.emplace_back(east_cell);
            }

            // Skip linking neighbor if we have no neighbor, prevent RNG out-of-bounds
            if (!neighbors.empty()) {
                auto random_index = static_cast<int>(rng(0, static_cast<int>(neighbors.size()) - 1));
                auto neighbor = neighbors.at(random_index);
                if (neighbor) {
                    c->link(c, neighbor, true);
                }
            }
        }

        return true;
    }

    return false;
}
