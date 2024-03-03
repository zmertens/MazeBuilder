#include "sidewinder.h"

#include <vector>
#include <random>
#include <functional>

#include "cell.h"
#include "grid.h"

using namespace mazes;
using namespace std;

/**
 * @param interactive = false
*/
bool sidewinder::run(grid& g, std::function<int(int, int)> const& get_int, bool interactive) noexcept {
    
    for (auto&& row : g.get_grid()) {
        vector<shared_ptr<cell>> run;
        for (auto&& c : row) {
            run.emplace_back(c);
            bool at_eastern_boundary {false}, at_northern_boundary {false};
            if (c->get_east() == nullptr)
                at_eastern_boundary = true;
            if (c->get_north() == nullptr)
                at_northern_boundary = true;
            bool should_close_out = at_eastern_boundary || (!at_northern_boundary && get_int(0, 1) == 0);

            if (should_close_out) {
                auto random_index {get_int(0, run.size() - 1)};
                auto&& member = run.at(random_index);
                if (member->get_north() != nullptr)
                    member->link(member, member->get_north(), true);
                run.clear();
            } else {
                c->link(c, c->get_east(), true);
            }
        }
    }

    return true;
}