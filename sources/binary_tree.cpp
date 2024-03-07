#include "binary_tree.h"

#include <memory>
#include <vector>
#include <random>
#include <functional>
#include <iostream>

#include "cell.h"
#include "grid.h"

/**
 * @param interactive = false
*/
bool mazes::binary_tree::run(mazes::grid_ptr& _grid, std::function<int(int, int)> const& get_int, bool interactive) noexcept {
    using namespace std;
    for (auto&& row : _grid->get_grid()) {
        for (auto&& c : row) {
            vector<shared_ptr<mazes::cell>> neighbors {};
            if (c->get_north() != nullptr)
                neighbors.emplace_back(c->get_north());
            if (c->get_east() != nullptr)
                neighbors.emplace_back(c->get_east());

            if (neighbors.empty())
                continue;
            
            auto random_index { get_int(0, neighbors.size() - 1) };
            auto&& neighbor = neighbors.at(random_index);
            c->link(c, neighbor, true);
        }
    }
    return true;
}
