#include <MazeBuilder/dfs.h>

#include <stack>
#include <algorithm>
#include <iterator>

#include <MazeBuilder/grid.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/randomizer.h>

using namespace mazes;
using namespace std;

/// @brief
/// @param g the grid to generate the maze on, and manipulate the cells
/// @param get_int 
/// @param rng 
/// @return 
bool dfs::run(const std::unique_ptr<grid_interface>& g, randomizer& rng) const noexcept {

    if (auto gg = dynamic_cast<grid*>(g.get())) {
     
        auto&& start = gg->search(rng(0, gg->num_cells() - 1));

        if (!start) {
            return false;
        }

        stack<shared_ptr<cell>> stack_of_cells;
        stack_of_cells.push(start);

        while (!stack_of_cells.empty()) {
            auto current_cell = stack_of_cells.top();
            auto current_neighbors = get_unvisited_neighbors(cref(current_cell));
            vector<shared_ptr<cell>> neighbors;
            // Copy neighbors unlinked (unvisited)
            copy_if(current_neighbors.cbegin(), current_neighbors.cend(), back_inserter(neighbors), [](const auto& n) {
                return n && n->get_links().empty();
                });

            if (neighbors.empty()) {
                stack_of_cells.pop();
            } else {
                // Mark current cell's neighbor as visited
                const auto& random_index = rng(0, neighbors.size() - 1);
                const auto& neighbor = neighbors.at(random_index);
                current_cell->link(neighbor);
                stack_of_cells.push(neighbor);
            }
        }
    } else {
        return false;
    }

    return true;
}

std::vector<std::shared_ptr<cell>> dfs::get_unvisited_neighbors(std::shared_ptr<cell> const& c) const noexcept {
    std::vector<std::shared_ptr<cell>> unvisited_neighbors;
    for (const auto& neighbor : c->get_neighbors()) {
        if (neighbor && neighbor->get_links().empty()) {
            unvisited_neighbors.push_back(neighbor);
        }
    }
    return unvisited_neighbors;
}
