#include "sidewinder.h"

#include <vector>
#include <functional>

#include "cell.h"
#include "grid.h"
#include "distance_grid.h"
#include "grid_interface.h"

using namespace mazes;
using namespace std;

/**
 * Generates a perfect maze if done correctly (no loops) by using "runs" to carve east-west
 * "Runs" are row-like passages that are carved by the sidewinder algorithm
 */
bool sidewinder::run(unique_ptr<grid_interface> const& _grid, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept {
    static vector<shared_ptr<cell>> store;
    static unsigned int last_row = 0;
    std::vector<shared_ptr<cell>> sorted_cells;

    // Cast grid_interface object to derived class
	if (auto grid_ptr = dynamic_cast<grid*>(_grid.get())) {
		grid_ptr->populate_vec(ref(sorted_cells));
		grid_ptr->sort_by_row_then_col(ref(sorted_cells));
    } else if (auto distance_grid_ptr = dynamic_cast<distance_grid*>(_grid.get())) {
		distance_grid_ptr->get_grid()->populate_vec(ref(sorted_cells));
		distance_grid_ptr->get_grid()->sort_by_row_then_col(ref(sorted_cells));
	} else {
		return false;
	}

    for (auto itr{ sorted_cells.cbegin() }; itr != sorted_cells.cend(); itr++) {
        if (itr->get()->get_row() != last_row) {
            last_row++;
            store.clear();
        }
        store.emplace_back(*itr);

        bool at_eastern_boundary{ false }, at_northern_boundary{ false };
        if (itr->get()->get_east() == nullptr)
            at_eastern_boundary = true;
        if (itr->get()->get_north() == nullptr)
            at_northern_boundary = true;
        // verify the sidewinder is reaching eastern wall and then flip a coin to go north or not
        bool should_close_out = at_eastern_boundary || (!at_northern_boundary && get_int(0, 1) == 0);
        if (should_close_out) {
            auto random_index{ get_int(0, store.size() - 1) };
            auto&& random_cell = store.at(random_index);
            if (random_cell->get_north() != nullptr) {
                random_cell->link(random_cell, random_cell->get_north(), true);
                store.clear();
            }
            // ensure last row is updated correctly
            last_row = itr->get()->get_row();
        } else if (!at_eastern_boundary) {
            itr->get()->link(*itr, itr->get()->get_east(), true);
        }
    }
    return true;
}

