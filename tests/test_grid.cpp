#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <functional>
#include <algorithm>
#include <sstream>
#include <random>
#include <memory>
#include <vector>
#include <numeric>

#include <MazeBuilder/grid.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/colored_grid.h>

#include <MazeBuilder/binary_tree.h>

using namespace mazes;
using namespace std;

static constexpr auto ROWS = 10, COLUMNS = 10, HEIGHT = 10;

static unique_ptr<grid> my_grid = make_unique<grid>(ROWS, COLUMNS, HEIGHT);
static unique_ptr<distance_grid> my_grid_distances = make_unique<distance_grid>(ROWS, COLUMNS, HEIGHT);
static unique_ptr<colored_grid> my_grid_colored = make_unique<colored_grid>(ROWS, COLUMNS, HEIGHT);

TEST_CASE("Assert grid", "[grid asserts]") {
    STATIC_REQUIRE(std::is_default_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::grid>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::grid>::value);

    STATIC_REQUIRE(std::is_default_constructible<mazes::distance_grid>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::distance_grid>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::distance_grid>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::distance_grid>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::distance_grid>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::distance_grid>::value);

    STATIC_REQUIRE(std::is_default_constructible<mazes::colored_grid>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::colored_grid>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::colored_grid>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::colored_grid>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::colored_grid>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::colored_grid>::value);
}

TEST_CASE( "Test grid future", "[grid future]" ) {

    SECTION(" Regular grid ") {
        auto [rows, columns, height] = my_grid->get_dimensions();

        REQUIRE(rows == ROWS);
        REQUIRE(columns == COLUMNS);
        REQUIRE(height == HEIGHT);
    }

    SECTION(" Distance grid ") {
        auto [rows, columns, height] = my_grid_distances->get_dimensions();

        REQUIRE(rows == ROWS);
        REQUIRE(columns == COLUMNS);
        REQUIRE(height == HEIGHT);
    }

    SECTION(" Colored grid ") {
        auto [rows, columns, height] = my_grid_colored->get_dimensions();

        REQUIRE(rows == ROWS);
        REQUIRE(columns == COLUMNS);
        REQUIRE(height == HEIGHT);
    }
}

/// @brief Verify that cells have been populated
TEST_CASE("Test to_vec", "[to_vec]") {

    // Each grid can be "futurized" only once
    REQUIRE(my_grid != nullptr);

    mt19937 mt{ std::random_device{}() };
    std::vector<int> shuffled_indices(ROWS * COLUMNS);
    std::iota(shuffled_indices.begin(), shuffled_indices.end(), 0);
    std::shuffle(shuffled_indices.begin(), shuffled_indices.end(), mt);

    my_grid->start_configuration(shuffled_indices);

    vector<shared_ptr<cell>> my_cells;
    my_grid->to_vec(ref(my_cells));

    REQUIRE(my_cells.size() == ROWS * COLUMNS);
}

TEST_CASE("Cells can link", "[cell link]") {

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

    SECTION("Bidirectional linking between cells") {
        cell1->link(cell2);

        REQUIRE(cell1->is_linked(cell2) == true);
        REQUIRE(cell2->is_linked(cell1) == true);
    }

    SECTION("Unidirectional linking between cells") {
        cell1->link(cell2, false);

        REQUIRE(cell1->is_linked(cell2) == true);
        REQUIRE(cell2->is_linked(cell1) == false);
    }

    SECTION("Unlinking cells") {
        cell1->link(cell2, true);
        REQUIRE(cell1->is_linked(cell2) == true);
        REQUIRE(cell2->is_linked(cell1) == true);

        cell1->unlink(cell2, true);
        cell2->unlink(cell1, true);
        REQUIRE_FALSE(cell1->is_linked(cell2));
        REQUIRE_FALSE(cell2->is_linked(cell1));
    }

    SECTION("Get links from a cell") {
        auto cell3 = std::make_shared<mazes::cell>(2);

        cell1->link(cell2, true);
        cell1->link(cell3, true);

        auto links = cell1->get_links();
        REQUIRE(links.size() == 2);
        REQUIRE(links.find(cell2) != links.end());
        REQUIRE(links.find(cell3) != links.end());
    }
}

TEST_CASE("Test cells get_neighbors method", "[get_neighbors]") {
    // Create cells
    auto cell0 = std::make_shared<cell>(0);
    auto cell1 = std::make_shared<cell>(1);
    auto cell2 = std::make_shared<cell>(2);
    auto cell3 = std::make_shared<cell>(3);
    auto cell4 = std::make_shared<cell>(4);

    // Set neighbors
    cell0->set_north(cell1);
    cell0->set_south(cell2);
    cell0->set_east(cell3);
    cell0->set_west(cell4);

    // Get neighbors
    auto neighbors = cell0->get_neighbors();

    // Verify neighbors
    REQUIRE(neighbors.size() == 4);
    REQUIRE(std::find(neighbors.begin(), neighbors.end(), cell1) != neighbors.end());
    REQUIRE(std::find(neighbors.begin(), neighbors.end(), cell2) != neighbors.end());
    REQUIRE(std::find(neighbors.begin(), neighbors.end(), cell3) != neighbors.end());
    REQUIRE(std::find(neighbors.begin(), neighbors.end(), cell4) != neighbors.end());

    SECTION("Linking neighbors") {
        cell1->set_north(cell2);
        REQUIRE(cell1->get_north() == cell2);
        cell2->set_south(cell1);
        REQUIRE(cell2->get_south() == cell1);

        cell1->link(cell2, true);
        REQUIRE(cell1->is_linked(cell2) == true);
        REQUIRE(cell2->is_linked(cell1) == true);
    }
}

