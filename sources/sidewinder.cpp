#include <MazeBuilder/sidewinder.h>

#include <functional>
#include <vector>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>

using namespace mazes;

/// @brief Generates a perfect maze if done correctly (no loops) by using "runs" to carve east-west
/// @details "Runs" are row-like passages that are carved by the sidewinder algorithm
/// @param g the grid to generate the maze on, and manipulate the cells
/// @param get_int
/// @param rng
bool sidewinder::run(grid_interface* g, randomizer& rng) const noexcept {
    using namespace std;

    vector<shared_ptr<cell>> store;
    unsigned int last_row = 0, row_counter = 0;
    std::vector<shared_ptr<cell>> cells;
    // Populate and presort cells
    g->to_vec(ref(cells));
    
    for (auto& itr : cells) {
        if (row_counter != last_row) {
            last_row = row_counter;
            store.clear();
        }
        store.push_back(itr);

        if (auto g_ptr = dynamic_cast<grid*>(g)) {
            bool at_eastern_boundary{ g_ptr->get_east(cref(itr)) == nullptr};
            bool at_northern_boundary{ g_ptr->get_north(cref(itr)) == nullptr };

            // Verify the sidewinder is reaching eastern wall and then flip a coin to go north or not
            bool should_close_out = at_eastern_boundary || (!at_northern_boundary && rng(0, 1) == 0);
            if (should_close_out) {
                auto random_index{ rng(0, store.size() - 1) };
                auto&& random_cell = store.at(random_index);
                if (!random_cell || !at_northern_boundary) {
                    lab::link(random_cell, g_ptr->get_east(random_cell));
                }
                store.clear();
            } else if (!at_eastern_boundary) {
                lab::link(itr, g_ptr->get_north(cref(itr)));
            }
        } else {
            // Invalid grid type
            return false;
        }
    }

    return true;
}

