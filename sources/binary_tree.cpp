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
    using namespace std;

	if (auto gg = dynamic_cast<grid*>(g.get())) {
        if (!gg) {
            return false;
        }

        stack<shared_ptr<grid::node>> node_stack;
        node_stack.push(gg->m_binary_search_tree_root);

        while (!node_stack.empty()) {
            auto current_node = node_stack.top();
            node_stack.pop();

            auto c = current_node->cell_ptr;
            if (c) {
                vector<shared_ptr<cell>> neighbors;
                auto north_cell = c->get_north();
                if (north_cell != nullptr) {
                    neighbors.emplace_back(north_cell);
                }
                auto east_cell = c->get_east();
                if (east_cell != nullptr) {
                    neighbors.emplace_back(east_cell);
                }

                // Skip linking neighbor if we have no neighbor, prevent RNG out-of-bounds
                if (!neighbors.empty()) {
                    auto random_index = static_cast<int>(get_int(0, neighbors.size() - 1));
                    auto neighbor = neighbors.at(random_index);
                    if (neighbor != nullptr) {
                        c->link(c, neighbor, true);
                    }
                }

                // Push right and left children to the stack
                if (current_node->right) {
                    node_stack.push(current_node->right);
                }
                if (current_node->left) {
                    node_stack.push(current_node->left);
                }
            }
        }

        return true;
    }

    return false;
}
