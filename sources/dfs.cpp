#include <MazeBuilder/dfs.h>

#include <stack>
#include <algorithm>
#include <iterator>

#include <MazeBuilder/grid.h>>
#include <MazeBuilder/cell.h>

using namespace mazes;
using namespace std;

/// @brief
/// @param g the grid to generate the maze on, and manipulate the cells
/// @param get_int 
/// @param rng 
/// @return 
bool dfs::run(const std::unique_ptr<grid_interface>& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept {

    if (auto gg = dynamic_cast<grid*>(g.get())) {
        if (!gg) {
            return false;
        }

        //auto [rows, columns, _] = gg->get_dimensions();
     
        auto&& start = gg->search(get_int(0, gg->num_cells() - 1));

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
                const auto& random_index = get_int(0, neighbors.size() - 1);
                const auto& neighbor = neighbors.at(random_index);
                current_cell->link(current_cell, neighbor);
                stack_of_cells.push(neighbor);
            }
        }
    } else {
        return false;
    }

    return true;
}
