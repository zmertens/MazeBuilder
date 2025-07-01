#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <unordered_set>
#include <vector>

#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>

using namespace mazes;
using namespace std;

static constexpr auto ROWS = 10, COLUMNS = 10, HEIGHT = 10;

static unique_ptr<grid> my_grid = make_unique<grid>(ROWS, COLUMNS, HEIGHT);
static unique_ptr<distance_grid> my_grid_distances = make_unique<distance_grid>(ROWS, COLUMNS, HEIGHT);
static unique_ptr<colored_grid> my_grid_colored = make_unique<colored_grid>(ROWS, COLUMNS, HEIGHT);

// Helper function to find a cell in the links of another cell
bool find_cell_in_links(const std::vector<std::pair<std::shared_ptr<cell>, bool>>& links, const std::shared_ptr<cell>& target) {
    return std::any_of(links.begin(), links.end(), [&target](const auto& link) {
        return link.first == target;
        });
}

TEST_CASE("Static Assert grid", "[grid static asserts]") {
    STATIC_REQUIRE(std::is_default_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::grid>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::grid>::value);


}

TEST_CASE( "Test grid dimensions", "[grid dimensions]" ) {

    SECTION(" Regular grid ") {
        auto [rows, columns, height] = my_grid_distances->operations().get_dimensions();

        REQUIRE(rows == ROWS);
        REQUIRE(columns == COLUMNS);
        REQUIRE(height == HEIGHT);
    }

    SECTION(" Distance grid ") {
        auto [rows, columns, height] = my_grid_distances->operations().get_dimensions();

        REQUIRE(rows == ROWS);
        REQUIRE(columns == COLUMNS);
        REQUIRE(height == HEIGHT);
    }

    SECTION(" Colored grid ") {
        auto [rows, columns, height] = my_grid_distances->operations().get_dimensions();

        REQUIRE(rows == ROWS);
        REQUIRE(columns == COLUMNS);
        REQUIRE(height == HEIGHT);
    }
}

// Verify that cells have been populated
TEST_CASE("Test to_vec", "[to_vec]") {

}

// Grid ops
TEST_CASE("Grid ops", "[grid operations work]") {


    SECTION("Get neighbors using grid methods") {
      
    }

    SECTION("Get all neighbors at once") {
      
    }

    SECTION("Edge cells have fewer neighbors") {
      
    }

    SECTION("Set and retrieve neighbors") {
     
    }
}

// Add a test for linking cells
TEST_CASE("Linking cells", "[lab links]") {
    
}

TEST_CASE("Test cell creation and lifecycle", "[cell lifecycle]") {


}
