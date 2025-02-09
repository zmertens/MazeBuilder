#include <MazeBuilder/sidewinder.h>

#include <vector>
#include <functional>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/grid_interface.h>

using namespace mazes;

/**
 * Generates a perfect maze if done correctly (no loops) by using "runs" to carve east-west
 * "Runs" are row-like passages that are carved by the sidewinder algorithm
 */
bool sidewinder::run(std::unique_ptr<grid_interface> const& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept {

    using namespace std;

    if (auto gg = dynamic_cast<grid*>(g.get())) {
        vector<shared_ptr<cell>> store;
        static unsigned int last_row = 0;
        std::vector<shared_ptr<cell>> cells;
        // Populate and presort cells
        gg->to_vec(ref(cells));
    
        for (auto& itr : cells) {
            if (itr->get_row() != last_row) {
                last_row = itr->get_row();
                store.clear();
            }
            store.push_back(itr);
    
            bool at_eastern_boundary{ itr->get_east() == nullptr };
            bool at_northern_boundary{ itr->get_north() == nullptr };
    
            // verify the sidewinder is reaching eastern wall and then flip a coin to go north or not
            bool should_close_out = at_eastern_boundary || (!at_northern_boundary && get_int(0, 1) == 0);
            if (should_close_out) {
                auto random_index{ get_int(0, store.size() - 1) };
                auto&& random_cell = store.at(random_index);
                if (!at_northern_boundary) {
                    random_cell->link(random_cell, random_cell->get_north(), true);
                }
                store.clear();
            } else if (!at_eastern_boundary) {
                itr->link(itr, itr->get_east(), true);
            }
        }
        return true;
    } else {
        return false;
    }
}

