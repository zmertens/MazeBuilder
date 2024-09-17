#include "dfs.h"

#include <stack>

#include "grid_interface.h"
#include "cell.h"

using namespace mazes;
using namespace std;

bool dfs::run(const std::unique_ptr<grid_interface>& _grid, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept {

    auto&& start = _grid->get_root();

    stack<shared_ptr<cell>> stack_of_cells;

    stack_of_cells.push(start);

    while (!stack_of_cells.empty()) {
        auto current_cell = stack_of_cells.top();

        auto neighbors = current_cell->get_neighbors();

        // Select neighbors unvisited / no links
        for (auto&& neighbor : neighbors) {
            if (neighbor->get_links().empty()) {
                neighbors.emplace_back(neighbor);
            }
        }

        if (neighbors.empty()) {
            stack_of_cells.pop();
        } else {
            auto next = neighbors[static_cast<size_t>(get_int(0, neighbors.size() - 1))];

            current_cell->link(current_cell, next);
            // Update current's visit history
            stack_of_cells.push(next);
        }
    }

    return true;
}
