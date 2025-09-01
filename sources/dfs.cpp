#include <MazeBuilder/dfs.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>

#include <algorithm>
#include <iterator>
#include <stack>
#include <unordered_set>

using namespace mazes;

/// @brief Generates maze structure by linking and manipulating the cells
/// @param g the grid to generate the maze on
/// @param rng the randomizer to use for selecting neighbors
/// @return
bool dfs::run(grid_interface* g, randomizer& rng) const noexcept {

    using namespace std;

    const auto& grid_ops = g->operations();

    auto start = grid_ops.search(rng(0, grid_ops.num_cells() - 1));

    if (!start) {
        return false;
    }

    stack<shared_ptr<cell>> stack_of_cells;
    unordered_set<shared_ptr<cell>> visited_cells;

    stack_of_cells.push(start);
    visited_cells.insert(start);

    while (!stack_of_cells.empty()) {
        auto current_cell = stack_of_cells.top();

        auto current_neighbors = grid_ops.get_neighbors(current_cell);

        // Filter out visited neighbors - use proper visited tracking
        vector<shared_ptr<cell>> unvisited_neighbors;

        copy_if(current_neighbors.begin(), current_neighbors.end(),
            back_inserter(unvisited_neighbors), [&visited_cells](const auto& n) {
                return n && visited_cells.find(n) == visited_cells.end();
            });

        if (unvisited_neighbors.empty()) {
            stack_of_cells.pop();
        } else {
            const auto& random_index = rng(0, static_cast<int>(unvisited_neighbors.size()) - 1);
            const auto& neighbor = unvisited_neighbors.at(random_index);

            // Use the lab class for linking
            lab::link(current_cell, neighbor, true);

            // Mark neighbor as visited and add to stack
            visited_cells.insert(neighbor);
            stack_of_cells.push(neighbor);
        }
    }

    return true;
}
