#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <functional>
#include <algorithm>
#include <sstream>
#include <random>
#include <memory>
#include <vector>

#include <MazeBuilder/grid.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/colored_grid.h>

#include <MazeBuilder/binary_tree.h>

using namespace mazes;
using namespace std;

static constexpr auto ROWS = 10, COLUMNS = 10, HEIGHT = 10;

static unique_ptr<grid_interface> my_grid = make_unique<grid>(ROWS, COLUMNS, HEIGHT);
static unique_ptr<grid_interface> my_grid_distances = make_unique<distance_grid>(ROWS, COLUMNS, HEIGHT);
static unique_ptr<grid_interface> my_grid_colored = make_unique<colored_grid>(ROWS, COLUMNS, HEIGHT);

TEST_CASE("Assert grid", "[grid asserts]") {
    STATIC_REQUIRE(std::is_default_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::grid>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::grid>::value);
}

TEST_CASE( "Test grid init", "[init]" ) {
    auto [rows, columns, height] = my_grid->get_dimensions();

    REQUIRE(rows == ROWS);
    REQUIRE(columns == COLUMNS);
    REQUIRE(height == HEIGHT);

    mt19937 rng{ 42681ul };
    static auto get_int = [&rng](int low, int high) ->int {
        uniform_int_distribution<int> dist{ low, high };
        return dist(rng);
        };

    SECTION("Run bt algorithm on grid", "[bt algo]") {
        binary_tree bt_algo;
        REQUIRE(bt_algo.run(ref(my_grid), cref(get_int), cref(rng)));
    }
}

/// @brief Verify that cells have been populated
TEST_CASE("Test to_vec", "[to_vec]") {
    //vector<shared_ptr<cell>> my_cells;
    //my_grid->to_vec(ref(my_cells));
    //unsigned int max{ 0 };
    //for (const auto& cell : my_cells) {
    //    max = (max < cell->get_index()) ? cell->get_index() : max;
    //    REQUIRE(cell->get_index() <= max);
    //}
}

TEST_CASE("Cells have neighbors", "[neighbors]") {

    // cell1 has cell2 neighbor to the south
    shared_ptr<cell> cell1{ make_shared<cell>(0) };
    shared_ptr<cell> cell2{ make_shared<cell>(1) };

    SECTION("Cell has neighbor to south") {
        cell1->set_south(cell2);
        REQUIRE(cell1->get_south() == cell2);
        REQUIRE(cell1->has_southern_neighbor());
        cell2->set_north(cell1);
        REQUIRE(cell2->has_northern_neighbor());
        auto&& neighbors = cell1->get_neighbors();
        REQUIRE(!neighbors.empty());
    }

    SECTION("Cells are linked") {
        // links are bi-directional by default
        cell1->link(cell1, cell2);
        REQUIRE(cell1->is_linked(cell2));
        REQUIRE(cell2->is_linked(cell1));
    }
}

