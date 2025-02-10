#include <MazeBuilder/binary_tree.h>

#include <memory>
#include <vector>
#include <functional>
#include <random>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid.h>

/**
 * @brief Generate maze in the direction of NORTH and EAST, starting in bottom-left corner of 2D grid
 *
 */
bool mazes::binary_tree::run(std::unique_ptr<grid_interface> const& _grid, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept {
	if (auto gg = dynamic_cast<grid*>(_grid.get())) {
        return this->run_on_cell(gg->m_binary_search_tree_root, std::cref(get_int), cref(rng));
    } else {
        return false;
    }
}

bool mazes::binary_tree::run_on_cell(std::shared_ptr<cell> const& _cell, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept {
    using namespace std;
    using namespace mazes;
    if (_cell != nullptr) {
        auto left_cell = _cell->get_left();
        if (left_cell != nullptr) {
            this->run_on_cell(left_cell, cref(get_int), cref(rng));
        }

        vector<shared_ptr<cell>> neighbors;
        auto north_cell = _cell->get_north();
        if (north_cell != nullptr) {
            neighbors.emplace_back(north_cell);
        }
        auto east_cell = _cell->get_east();
        if (east_cell != nullptr) {
            neighbors.emplace_back(east_cell);
        }

        // skip linking neighbor if we have no neighbor, prevent RNG out-of-bounds
        if (!neighbors.empty()) {
            auto random_index = static_cast<int>(get_int(0, neighbors.size() - 1));
            auto neighbor = neighbors.at(random_index);
            if (neighbor != nullptr) {
                _cell->link(_cell, neighbor, true);
            }
        }

        auto right_cell = _cell->get_right();
        if (right_cell != nullptr) {
            this->run_on_cell(right_cell, cref(get_int), cref(rng));
        }
    } // if
    return true;
}