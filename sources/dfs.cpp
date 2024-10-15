#include <MazeBuilder/dfs.h>

#include <stack>
#include <algorithm>
#include <iterator>

#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/cell.h>

using namespace mazes;
using namespace std;

/**
 * @brief Runs the Depth-First Search (DFS) algorithm on the provided grid.
 *
 * @param _grid A unique pointer to the grid interface.
 * @param get_int A function to generate random integers.
 * @param rng A random number generator.
 * @return true if the DFS algorithm runs successfully, false otherwise.
 */
bool dfs::run(const std::unique_ptr<grid_interface>& _grid, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept {

    auto&& start = _grid->search(_grid->get_root(), get_int(0, _grid->get_rows() * _grid->get_columns()));

    if (!start) {
        return false;
    }

    stack<shared_ptr<cell>> stack_of_cells;
    stack_of_cells.push(start);

    while (!stack_of_cells.empty()) {
        auto current_cell = stack_of_cells.top();
        auto current_neighbors = current_cell->get_neighbors();
        vector<shared_ptr<cell>> neighbors;
        // Copy neighbors unlinked (unvisited)
        copy_if(current_neighbors.cbegin(), current_neighbors.cend(), back_inserter(neighbors), [](const auto& n) {
            return n && n->get_links().empty();
            });

        if (neighbors.empty()) {
            stack_of_cells.pop();
        } else {
			// Mark current cell's neighbor as visited
			auto&& random_index = get_int(0, neighbors.size() - 1);
			auto&& neighbor = neighbors.at(random_index);
			current_cell->link(current_cell, neighbor);
			stack_of_cells.push(neighbor);
        }
    }

    return true;
}
