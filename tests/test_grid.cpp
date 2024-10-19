#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <functional>
#include <sstream>
#include <random>
#include <memory>
#include <vector>

#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/grid.h>
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
}

TEST_CASE( "Test grid insertion and search", "[insert and search]" ) {
    auto my_cell = make_shared<cell>(0, 0, 0);
    my_grid->insert(my_grid->get_root(), my_cell->get_index());
    auto result = my_grid->search(my_grid->get_root(), 0);
    REQUIRE(result->get_index() == my_cell->get_index());
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

