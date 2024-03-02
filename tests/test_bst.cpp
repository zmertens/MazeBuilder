#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <algorithm>
#include <functional>
#include <random>

#include "grid.h"
#include "bst.h"

using namespace std;

static auto get_int = [](int low, int high) ->int {
    random_device rd;
    seed_seq seed {rd()};
    mt19937 mt(seed);
    uniform_int_distribution<int> dist {low, high};
    return dist(mt);
};

TEST_CASE("Binary Search Tree maze algo computed", "[bst]") {
    grid g {5, 5};
    bst my_bst_maze;
    SECTION("Run BST and check for success", "![benchmark]") {
        bool success = my_bst_maze.run(g, get_int);
        REQUIRE(success == true);
        stringstream ss;
        ss << g;
        REQUIRE(ss.str().length() != 0);
    }
}

TEST_CASE("Cells have neighbors", "[cells]") {

    using namespace std;

    auto is_equal = [&](shared_ptr<cell> cell1, shared_ptr<cell> cell2) -> bool {
        return cell1 == cell2;
    };

    // cell1 has cell2 neighbor to the south
    shared_ptr<cell> cell1 {make_shared<cell>(0, 0)};
    shared_ptr<cell> cell2 {make_shared<cell>(0, 1)};

    SECTION("Cell has neighbor to south") {
        cell1->set_south(cell2);
        REQUIRE(is_equal(cell1->get_south(), cell2));
    }

    SECTION("Cells are linked") {
        // links are bi-directional by default
        cell1->link(cell1, cell2);
        REQUIRE(cell1->is_linked(cell2));
    }

    SECTION("Cell1 has neighbors") {
        cell1->set_south(cell2);
        auto&& neighbors = cell1->get_neighbors();
        REQUIRE(!neighbors.empty());
    }
}

