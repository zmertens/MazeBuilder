#include <MazeBuilder/binary_tree.h>

#include <memory>
#include <vector>
#include <functional>
#include <stack>
#include <random>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid.h>

/// @brief Generate maze in the direction of NORTH and EAST, starting in bottom - left corner of 2D grid
/// @param _grid 
/// @param get_int 
/// @param rng 
/// @return 
bool mazes::binary_tree::run(std::unique_ptr<grid_interface> const& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept {
    if (auto gg = dynamic_cast<grid*>(g.get())) {
        if (!gg) {
            return false;  // This check is redundant since dynamic_cast already checked
        }

        // Clear the existing links so we start with a fresh maze
        auto [rows, columns, _] = gg->get_dimensions();
        std::vector<std::shared_ptr<cell>> cells;
        cells.reserve(rows * columns);
        gg->to_vec(cells);

        // Clear existing links
        for (auto& cell : cells) {
            for (auto& neighbor : cell->get_neighbors()) {
                if (neighbor) {
                    cell->unlink(cell, neighbor, true);
                }
            }
        }

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
                auto random_index = static_cast<int>(get_int(0, static_cast<int>(neighbors.size()) - 1));
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
