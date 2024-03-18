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
bool mazes::binary_tree::run(std::unique_ptr<grid>& _grid, std::function<int(int, int)> const& get_int, bool interactive) noexcept {
	return this->run_on_cell(_grid->get_root(), get_int);
}

bool mazes::binary_tree::run_on_cell(std::shared_ptr<cell>& _cell, std::function<int(int, int)> const& get_int) noexcept {
    using namespace std;
    using namespace mazes;
    if (_cell != nullptr) {
        this->run_on_cell(_cell->get_left(), get_int);
        vector<shared_ptr<cell>> neighbors;
        if (_cell->get_north() != nullptr) {
            neighbors.emplace_back(_cell->get_north());
        }
        if (_cell->get_east() != nullptr) {
            neighbors.emplace_back(_cell->get_east());
        }
        // skip linking neighbor if we have no neighbor, prevent RNG out-of-bounds
        if (!neighbors.empty()) {
            auto&& random_index = static_cast<size_t>(get_int(0, neighbors.size() - 1));
            auto&& neighbor = neighbors.at(random_index);
            if (neighbor != nullptr)
                _cell->link(_cell, neighbor, true);
        }
        this->run_on_cell(_cell->get_right(), get_int);
    } // if
    return true;
}