#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <functional>

#include "grid.h"
#include "grid_interface.h"
#include "distance_grid.h"

using namespace mazes;
using namespace std;

static shared_ptr<grid> my_grid = make_shared<grid>(10, 10, 10);

TEST_CASE( "Test grid init", "[init]" ) {
    REQUIRE(my_grid->get_rows() == 10);
    REQUIRE(my_grid->get_columns() == 10);
}

TEST_CASE( "Test grid insertion and search", "[insert and search]" ) {
    auto my_cell = make_shared<cell>(0, 0, 0);
    my_grid->insert(my_grid->get_root(), 
        my_cell->get_row(), my_cell->get_column(), 
        my_cell->get_index());
    auto result = my_grid->search(my_grid->get_root(), 0);
    REQUIRE(result->get_index() == my_cell->get_index());
}

TEST_CASE( "Test grid deletion ", "[delete]" ) {
    auto my_cell = make_shared<cell>(0, 0, 0);
    my_grid->insert(my_grid->get_root(), 
        my_cell->get_row(), my_cell->get_column(), 
        my_cell->get_index());
    my_grid->del(my_grid->get_root(), 0);
    auto result = my_grid->search(my_grid->get_root(), 0);
    REQUIRE(result != my_cell);
}

TEST_CASE( "Test grid update ", "[update]" ) {
    auto my_cell = make_shared<cell>(0, 0, 0);
    my_grid->insert(my_grid->get_root(), 
        my_cell->get_row(), my_cell->get_column(), 
        my_cell->get_index());
    my_cell->set_index(1);
    auto result = my_grid->search(my_grid->get_root(), 1);
    REQUIRE(result->get_index() == 1);
    REQUIRE(result->get_index() == my_cell->get_index());
}

TEST_CASE( "Test appending grids", "[append]") {
    auto my_grid2 = make_unique<grid>(10, 10, 10);
    my_grid2->insert(my_grid2->get_root(), 0, 0, 0);
    my_grid2->insert(my_grid2->get_root(), 1, 1, 1);
    my_grid2->insert(my_grid2->get_root(), 2, 2, 2);
    my_grid->append(my_grid2);

    REQUIRE(my_grid->search(my_grid->get_root(), 0) != nullptr);
    REQUIRE(my_grid->search(my_grid->get_root(), 1) != nullptr);
    REQUIRE(my_grid->search(my_grid->get_root(), 2) != nullptr);
    REQUIRE(my_grid->search(my_grid->get_root(), 3) != nullptr);
}

TEST_CASE("Test distance grid", "[distance grid]") {
	//auto my_distance_grid = make_unique<distance_grid>(10, 10, 10);
	//auto my_cell = make_shared<cell>(0, 0, 0);
	//my_distance_grid->insert(my_distance_grid->get_root(),
	//	my_cell->get_row(), my_cell->get_column(),
	//	my_cell->get_index());
	//auto result = my_distance_grid->search(my_distance_grid->get_root(), 0);
	//REQUIRE(result->get_index() == my_cell->get_index());
}