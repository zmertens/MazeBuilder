#include "sidewinder.h"

#include <vector>
#include <functional>

#include "cell.h"
#include "grid.h"

using namespace mazes;
using namespace std;

/**
 * @param interactive = false
*/
bool sidewinder::run(unique_ptr<grid> const& _grid, std::function<int(int, int)> const& get_int, bool interactive) const noexcept {
    return this->run_on_cell(_grid->get_root(), get_int);
}

bool mazes::sidewinder::run_on_cell(std::shared_ptr<cell> const& _cell, std::function<int(int, int)> const& get_int) const noexcept {
    static vector<shared_ptr<cell>> store;
    if (_cell != nullptr) {
        this->run_on_cell(_cell->get_left(), get_int);
        store.clear();
        store.emplace_back(_cell);
        bool at_eastern_boundary{ false }, at_northern_boundary{ false };
        if (_cell->get_east() == nullptr)
            at_eastern_boundary = true;
        if (_cell->get_north() == nullptr)
            at_northern_boundary = true;
        // verify the sidewinder is reaching eastern wall and the RNG helps check storage size
        bool should_close_out = at_eastern_boundary || (!at_northern_boundary && get_int(0, 1) == 0);
        if (should_close_out) {
            auto random_index{ get_int(0, store.size() - 1) };
            auto&& random_cell = store.at(random_index);
            if (random_cell->get_north() != nullptr) {
                random_cell->link(random_cell, random_cell->get_north(), true);
                store.clear();
            } else {
                _cell->link(_cell, _cell->get_east(), true);
            }
        }
        this->run_on_cell(_cell->get_right(), get_int);
    }
    return true;
}