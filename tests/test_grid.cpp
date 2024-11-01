#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <functional>
#include <sstream>
#include <random>
#include <memory>
#include <vector>

#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/distances.h>
#include <MazeBuilder/maze_factory.h>

using namespace mazes;
using namespace std;

static unique_ptr<grid_interface> my_grid = make_unique<grid>(10, 10, 10);

TEST_CASE( "Test grid init", "[init]" ) {
    REQUIRE(my_grid->get_rows() == 10);
    REQUIRE(my_grid->get_columns() == 10);
    SECTION("Make a very large grid", "[large grid]") {
        mt19937 rng{ 42681ul };
        static auto get_int = [&rng](int low, int high) ->int {
            uniform_int_distribution<int> dist{ low, high };
            return dist(rng);
            };
        my_grid = make_unique<grid>(1'000, 1'000);
        binary_tree bt_algo;
        REQUIRE(bt_algo.run(ref(my_grid), cref(get_int), cref(rng)));
    }
}

TEST_CASE( "Test grid insertion and search", "[insert and search]" ) {
    auto my_cell = make_shared<cell>(0, 0, 0);
    my_grid->insert(my_grid->get_root(), my_cell->get_index());
    auto result = my_grid->search(my_grid->get_root(), 0);
    REQUIRE(result->get_index() == my_cell->get_index());
}

TEST_CASE("Searching the grid yields positive results", "[search]") {
    unsigned int rows{ 25 }, columns{ 20 };
    vector<unsigned int> shuffled_ints{ rows * columns };
    shuffled_ints.reserve(rows * columns);
    int next_index{ 0 };
    for (auto itr{ shuffled_ints.begin() }; itr != shuffled_ints.end(); itr++) {
        *itr = next_index++;
    }

    auto rd = std::random_device{};
    auto rng = std::default_random_engine{ rd() };
    shuffle(begin(shuffled_ints), end(shuffled_ints), rng);

    unique_ptr<grid> _grid{ make_unique<grid>(rows, columns) };

    for (auto&& i : shuffled_ints) {
        auto&& found = _grid->search(_grid->get_root(), i);
        REQUIRE(found != nullptr);
    }
}

TEST_CASE( "Test grid deletion ", "[delete]" ) {
    auto my_cell = make_shared<cell>(0, 0, 0);
    my_grid->insert(my_grid->get_root(), my_cell->get_index());
    my_grid->del(my_grid->get_root(), 0);
    auto result = my_grid->search(my_grid->get_root(), 0);
    REQUIRE(result != my_cell);
}

TEST_CASE( "Test grid update ", "[update]" ) {
	auto my_cell = my_grid->search(my_grid->get_root(), 0);
    REQUIRE(my_cell);
    auto&& modify_this_index = my_cell->get_index();
    modify_this_index = my_grid->get_columns() - 1;
	bool success = my_grid->update(my_cell, my_cell->get_index(), modify_this_index);
	REQUIRE(success);
	auto result = my_grid->search(my_grid->get_root(), modify_this_index);
    REQUIRE(result != nullptr);
    REQUIRE(result->get_index() == my_cell->get_index());
}

TEST_CASE( "Test appending grids", "[append]") {
    shared_ptr<grid_interface> my_grid2 = make_unique<grid>(10, 10, 10);
    my_grid2->insert(my_grid2->get_root(), 0);
    my_grid2->insert(my_grid2->get_root(), 1);
    my_grid2->insert(my_grid2->get_root(), 2);
    my_grid->append(my_grid2);

    REQUIRE(my_grid->search(my_grid->get_root(), 0) != nullptr);
    REQUIRE(my_grid->search(my_grid->get_root(), 1) != nullptr);
    REQUIRE(my_grid->search(my_grid->get_root(), 2) != nullptr);
    REQUIRE(my_grid->search(my_grid->get_root(), 3) != nullptr);
}

TEST_CASE("Test distance grid", "[distance grid output]") {
	auto my_distance_grid = make_unique<distance_grid>(10, 10, 10);
	auto my_cell = make_shared<cell>(0, 0, 0);
	my_distance_grid->insert(my_distance_grid->get_root(), my_cell->get_index());
	auto result = my_distance_grid->search(my_distance_grid->get_root(), 0);
	REQUIRE(result->get_index() == my_cell->get_index());
	stringstream ss;
	ss << my_distance_grid.get();
	REQUIRE(ss.str().size() > 0);
}

TEST_CASE("Grids are sortable", "[sort]") {
    unique_ptr<grid> g1{ make_unique<grid>(100, 100) };
    vector<shared_ptr<cell>> sorted_cells;
    g1->make_sorted_vec(ref(sorted_cells));
    // each sorted cell should increase in index value up until the max in the grid
    unsigned int max{ 0 };
    for (auto&& cell : sorted_cells) {
        max = cell->get_index();
        REQUIRE(cell->get_index() >= max);
    }
}

TEST_CASE("Cells have neighbors", "[cells]") {

    // cell1 has cell2 neighbor to the south
    shared_ptr<cell> cell1{ make_shared<cell>(0, 0, 0) };
    shared_ptr<cell> cell2{ make_shared<cell>(0, 1, 1) };

    SECTION("Cell has neighbor to south") {
        cell1->set_south(cell2);
        REQUIRE(cell1->get_south() == cell2);
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

