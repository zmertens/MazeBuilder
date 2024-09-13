#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <functional>
#include <sstream>
#include <random>

#include "binary_tree.h"
#include "grid.h"
#include "grid_interface.h"
#include "distance_grid.h"
#include "distances.h"

using namespace mazes;
using namespace std;

static shared_ptr<grid> my_grid = make_shared<grid>(10, 10, 10);

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
    auto my_cell = make_shared<cell>(0, 0, 0);
    my_grid->insert(my_grid->get_root(), my_cell->get_index());
    my_cell->set_index(1);
    auto result = my_grid->search(my_grid->get_root(), 1);
    REQUIRE(result->get_index() == 1);
    REQUIRE(result->get_index() == my_cell->get_index());
}

TEST_CASE( "Test appending grids", "[append]") {
    auto my_grid2 = make_unique<grid>(10, 10, 10);
    my_grid2->insert(my_grid2->get_root(), 0);
    my_grid2->insert(my_grid2->get_root(), 1);
    my_grid2->insert(my_grid2->get_root(), 2);
    my_grid->append(my_grid2);

    REQUIRE(my_grid->search(my_grid->get_root(), 0) != nullptr);
    REQUIRE(my_grid->search(my_grid->get_root(), 1) != nullptr);
    REQUIRE(my_grid->search(my_grid->get_root(), 2) != nullptr);
    REQUIRE(my_grid->search(my_grid->get_root(), 3) != nullptr);
}

TEST_CASE("Test distance grid", "[distance grid]") {
    static constexpr auto ROW = 10, COL = 10, HEIGHT = 10;
    unique_ptr<grid_interface> my_distance_grid = make_unique<distance_grid>(ROW, COL, HEIGHT);

    my_distance_grid->insert(my_distance_grid->get_root(), 0);
	my_distance_grid->insert(my_distance_grid->get_root(), ROW + COL * (ROW - 1));

	auto&& start = my_distance_grid->search(my_distance_grid->get_root(), 0);
    auto&& end = my_distance_grid->search(my_distance_grid->get_root(), 1);

    REQUIRE(start != end);

    mt19937 rng{ 42681ul };
    auto get_int = [&rng](int low, int high) ->int {
        uniform_int_distribution<int> dist{ low, high };
        return dist(rng);
    };
	binary_tree bt_algo;
	REQUIRE(bt_algo.run(ref(my_distance_grid), cref(get_int), cref(rng)));

    auto dists = make_shared<distances>(my_distance_grid->get_root());

	dynamic_cast<distance_grid*>(my_distance_grid.get())->set_distances(dists);

	auto&& path = start->distances()->path_to(end);

	REQUIRE(path);
	//REQUIRE(path->get_cells().size() >= 2);

	if (auto db = dynamic_cast<distance_grid*>(my_distance_grid.get()); db != nullptr) {
		auto&& dg = db->get_distances();
		REQUIRE(dg->operator[](end) == 1);
        stringstream ss;
		ss << db;
        REQUIRE(!ss.str().empty());
	}
}