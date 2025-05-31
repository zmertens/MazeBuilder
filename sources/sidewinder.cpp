#include <MazeBuilder/sidewinder.h>

#include <vector>
#include <functional>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_interface.h>
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
    
        bool at_eastern_boundary{ itr->get_east() == nullptr };
        bool at_northern_boundary{ itr->get_north() == nullptr };
    
        // Verify the sidewinder is reaching eastern wall and then flip a coin to go north or not
        bool should_close_out = at_eastern_boundary || (!at_northern_boundary && rng(0, 1) == 0);
        if (should_close_out) {
            auto random_index{ rng(0, store.size() - 1) };
            auto&& random_cell = store.at(random_index);
            if (!random_cell || !at_northern_boundary) {
                random_cell->link(random_cell, random_cell->get_north());
            }
            store.clear();
        } else if (!at_eastern_boundary) {
            itr->link(itr, itr->get_east());
        }
    }

    return true;
}

