#include <MazeBuilder/dfs.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>

#include <algorithm>
#include <iterator>
#include <stack>

using namespace mazes;
using namespace std;

/// @brief Generates maze structure by linking and manipulating the cells
/// @param g the grid to generate the maze on
/// @param rng the randomizer to use for selecting neighbors
/// @return
bool dfs::run(grid_interface* g, randomizer& rng) const noexcept {
    if (auto gg = dynamic_cast<grid*>(g)) {
        auto start = gg->search(rng(0, gg->num_cells() - 1));

        if (!start) {
            return false;
        }

        stack<shared_ptr<cell>> stack_of_cells;
        stack_of_cells.push(start);

        while (!stack_of_cells.empty()) {
            auto current_cell = stack_of_cells.top();

            // Get neighbors using grid's function instead of cell's
            auto current_neighbors = gg->get_neighbors(current_cell);
            vector<shared_ptr<cell>> unvisited_neighbors;

            copy_if(current_neighbors.begin(), current_neighbors.end(),
                back_inserter(unvisited_neighbors), [](const auto& n) {
                    return n && n->get_links().empty();
                });

            if (unvisited_neighbors.empty()) {
                stack_of_cells.pop();
            } else {
                const auto& random_index = rng(0, unvisited_neighbors.size() - 1);
                const auto& neighbor = unvisited_neighbors.at(random_index);

                // Use the lab class for linking
                lab::link(current_cell, neighbor, true);

                stack_of_cells.push(neighbor);
            }
        }
    } else {
        return false;
    }

    return true;
}

std::vector<std::shared_ptr<cell>> dfs::get_unvisited_neighbors(std::shared_ptr<cell> const& c, const grid* g) const noexcept {
    std::vector<std::shared_ptr<cell>> unvisited_neighbors;

    if (!c || !g) return unvisited_neighbors;

    auto neighbors = g->get_neighbors(c);

    for (const auto& neighbor : neighbors) {
        if (neighbor && neighbor->get_links().empty()) {
            unvisited_neighbors.push_back(neighbor);
        }
    }

    return unvisited_neighbors;
}
