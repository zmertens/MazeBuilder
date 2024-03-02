#include "bst.h"

#include <vector>
#include <random>
#include <functional>

#include "cell.h"
#include "grid.h"

using namespace std;

bool bst::run(grid& g, std::function<int(int, int)> const& get_int) const noexcept {

    for (auto&& row : g.get_grid()) {
        for (auto&& c : row) {
            vector<shared_ptr<cell>> neighbors {};
            if (c->get_north() != nullptr)
                neighbors.emplace_back(c->get_north());
            if (c->get_east() != nullptr)
                neighbors.emplace_back(c->get_east());

            if (neighbors.empty())
                continue;

            auto random_index { get_int(0, neighbors.size() - 1) };
            // cout << "random_index: " << random_index << endl;
            auto&& neighbor = neighbors.at(random_index);
            
            c->link(c, neighbor, true);
        }
    }
    return true;
}