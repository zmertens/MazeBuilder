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
	return this->run_on_cell(_grid->get_root(), std::cref(get_int), cref(rng));
}

bool mazes::binary_tree::run_on_cell(std::shared_ptr<cell> const& _cell, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept {
    using namespace std;
    using namespace mazes;
    if (_cell != nullptr) {
        this->run_on_cell(_cell->get_left(), cref(get_int), cref(rng));
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
        this->run_on_cell(_cell->get_right(), cref(get_int), cref(rng));
    } // if
    return true;
}
