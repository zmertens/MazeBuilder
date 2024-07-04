#include "sidewinder.h"

#include <vector>
#include <functional>

#include "cell.h"
#include "grid.h"

using namespace mazes;
using namespace std;

/**
 * Generates a perfect maze if done correctly (no loops) by using "runs" to carve east-west
 * @param interactive = false
*/
bool sidewinder::run(unique_ptr<grid> const& _grid, std::function<int(int, int)> const& get_int, bool interactive) const noexcept {
    static vector<shared_ptr<cell>> store;
    static unsigned int last_row = 0;
    std::vector<shared_ptr<cell>> sorted_cells;
    _grid->populate_vec(ref(sorted_cells));
    _grid->sort_by_row_then_col(ref(sorted_cells));
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

//bool sidewinder::run_on_cell(std::shared_ptr<cell> const& _cell, std::function<int(int, int)> const& get_int) const noexcept {
//    static vector<shared_ptr<cell>> store;
//    static unsigned int last_row = 0;
//    if (_cell != nullptr) {
//        this->run_on_cell(_cell->get_left(), get_int);
//        if (_cell->get_row() != last_row) {
//            last_row++;
//            store.clear();
//        }
//        store.emplace_back(_cell);
//        bool at_eastern_boundary{ false }, at_northern_boundary{ false };
//        if (_cell->get_east() == nullptr)
//            at_eastern_boundary = true;
//        if (_cell->get_north() == nullptr)
//            at_northern_boundary = true;
//        // verify the sidewinder is reaching eastern wall and the RNG helps check storage size
//        bool should_close_out = at_eastern_boundary || (!at_northern_boundary && get_int(0, 1) == 0);
//        if (should_close_out) {
//            auto random_index{ get_int(0, store.size() - 1) };
//            auto&& random_cell = store.at(random_index);
//            if (random_cell->get_north() != nullptr) {
//                random_cell->link(random_cell, random_cell->get_north(), true);
//                store.clear();
//            }
//            else if (random_cell->get_east() != nullptr) {
//                random_cell->link(random_cell, random_cell->get_east(), true);
//            }
//        }
//        this->run_on_cell(_cell->get_right(), get_int);
//    }
//    return true;
//} // run_on_cells
