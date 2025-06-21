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

/// @brief Helper function to find a cell in a vector of links
/// @param links The vector of links to search
/// @param target The cell to find
/// @return True if the cell is found, false otherwise
bool find_cell_in_links(const std::vector<std::pair<std::shared_ptr<cell>, bool>>& links, const std::shared_ptr<cell>& target) {
    return std::any_of(links.begin(), links.end(), [&target](const auto& link) {
        return link.first == target;
        });
}

TEST_CASE("Assert grid", "[grid asserts]") {
    STATIC_REQUIRE(std::is_default_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::grid>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::grid>::value);

    //STATIC_REQUIRE(std::is_default_constructible<mazes::distance_grid>::value);
    //STATIC_REQUIRE(std::is_destructible<mazes::distance_grid>::value);
    //STATIC_REQUIRE(std::is_copy_constructible<mazes::distance_grid>::value);
    //STATIC_REQUIRE(std::is_copy_assignable<mazes::distance_grid>::value);
    //STATIC_REQUIRE(std::is_move_constructible<mazes::distance_grid>::value);
    //STATIC_REQUIRE(std::is_move_assignable<mazes::distance_grid>::value);

    //STATIC_REQUIRE(std::is_default_constructible<mazes::colored_grid>::value);
    //STATIC_REQUIRE(std::is_destructible<mazes::colored_grid>::value);
    //STATIC_REQUIRE(std::is_copy_constructible<mazes::colored_grid>::value);
    //STATIC_REQUIRE(std::is_copy_assignable<mazes::colored_grid>::value);
    //STATIC_REQUIRE(std::is_move_constructible<mazes::colored_grid>::value);
    //STATIC_REQUIRE(std::is_move_assignable<mazes::colored_grid>::value);
}

TEST_CASE( "Test grid dimensions", "[grid dimensions]" ) {

    //SECTION(" Regular grid ") {
    //    auto [rows, columns, height] = my_grid->get_dimensions();

    //    REQUIRE(rows == ROWS);
    //    REQUIRE(columns == COLUMNS);
    //    REQUIRE(height == HEIGHT);
    //}

    //SECTION(" Distance grid ") {
    //    auto [rows, columns, height] = my_grid_distances->get_dimensions();

    //    REQUIRE(rows == ROWS);
    //    REQUIRE(columns == COLUMNS);
    //    REQUIRE(height == HEIGHT);
    //}

    //SECTION(" Colored grid ") {
    //    auto [rows, columns, height] = my_grid_colored->get_dimensions();

    //    REQUIRE(rows == ROWS);
    //    REQUIRE(columns == COLUMNS);
    //    REQUIRE(height == HEIGHT);
    //}
}

/// @brief Verify that cells have been populated
TEST_CASE("Test to_vec", "[to_vec]") {

    randomizer rng;
    rng.seed();
    auto shuffled_indices = rng.get_num_ints_incl(0, ROWS * COLUMNS);

    //my_grid->configure(cref(shuffled_indices));
    //my_grid_distances->configure(cref(shuffled_indices));
    //my_grid_colored->configure(cref(shuffled_indices));

    //vector<shared_ptr<cell>> my_cells;
    //my_grid->to_vec(ref(my_cells));

    //REQUIRE(my_cells.size() == ROWS * COLUMNS);

    //my_cells.clear();
    //my_grid_distances->to_vec(ref(my_cells));
    //REQUIRE(my_cells.size() == ROWS * COLUMNS);

    //my_cells.clear();
    //my_grid_colored->to_vec(ref(my_cells));
    //REQUIRE(my_cells.size() == ROWS * COLUMNS);
}

// Add a new test case for grid neighbor handling
TEST_CASE("Grid neighbor handling", "[grid neighbors]") {
    // Create a small test grid
    auto test_grid = std::make_shared<grid>(3, 3, 1);
    std::vector<int> indices = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    test_grid->configure(indices);

    // Test getting a cell by index
    auto center_cell = test_grid->search(4); // Middle cell in 3x3 grid
    REQUIRE(center_cell != nullptr);
    REQUIRE(center_cell->get_index() == 4);

    SECTION("Get neighbors using grid methods") {
        // Center cell should have 4 neighbors (N, S, E, W)
        auto north = test_grid->get_north(center_cell);
        auto south = test_grid->get_south(center_cell);
        auto east = test_grid->get_east(center_cell);
        auto west = test_grid->get_west(center_cell);

        REQUIRE(north != nullptr);
        REQUIRE(south != nullptr);
        REQUIRE(east != nullptr);
        REQUIRE(west != nullptr);

        // Verify indices based on the expected layout
        REQUIRE(north->get_index() == 1);
        REQUIRE(south->get_index() == 7);
        REQUIRE(east->get_index() == 5);
        REQUIRE(west->get_index() == 3);
    }

    SECTION("Get all neighbors at once") {
        auto neighbors = test_grid->get_neighbors(center_cell);
        REQUIRE(neighbors.size() == 4);

        // Verify all expected neighbors are in the list
        std::vector<int> expected_indices = { 1, 7, 5, 3 };
        for (const auto& neighbor : neighbors) {
            REQUIRE(std::find(expected_indices.begin(), expected_indices.end(),
                neighbor->get_index()) != expected_indices.end());
        }
    }

    SECTION("Edge cells have fewer neighbors") {
        auto corner_cell = test_grid->search(0); // Top-left corner
        auto neighbors = test_grid->get_neighbors(corner_cell);

        // Corner should only have 2 neighbors (E, S)
        REQUIRE(neighbors.size() == 2);

        std::vector<int> expected_indices = { 1, 3 };
        for (const auto& neighbor : neighbors) {
            REQUIRE(std::find(expected_indices.begin(), expected_indices.end(),
                neighbor->get_index()) != expected_indices.end());
        }
    }

    SECTION("Set and retrieve neighbors") {
        auto cell0 = test_grid->search(0);
        auto cell1 = test_grid->search(1);

        REQUIRE(cell0 != nullptr);
        REQUIRE(cell1 != nullptr);

        // Set custom neighbor
        test_grid->set_neighbor(cell0, Direction::North, cell1);

        // Verify the neighbor was set correctly
        auto retrieved = test_grid->get_neighbor(cell0, Direction::North);
        REQUIRE(retrieved == cell1);
    }
}

// Add a test for linking cells through the lab namespace
TEST_CASE("Linking cells through lab namespace", "[lab links]") {
    auto test_grid = std::make_shared<grid>(3, 3, 1);
    std::vector<int> indices = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    test_grid->configure(indices);

    auto cell0 = test_grid->search(0);
    auto cell1 = test_grid->search(1);

    REQUIRE(cell0 != nullptr);
    REQUIRE(cell1 != nullptr);

    // Test that cells aren't linked initially
    REQUIRE_FALSE(cell0->is_linked(cell1));
    REQUIRE_FALSE(cell1->is_linked(cell0));

    // Link cells using lab::link and verify
    lab::link(cell0, cell1);
    REQUIRE(cell0->is_linked(cell1));
    REQUIRE(cell1->is_linked(cell0));

    // Verify links with get_links()
    auto links0 = cell0->get_links();
    auto links1 = cell1->get_links();
    
    REQUIRE(find_cell_in_links(links0, cell1));
    REQUIRE(find_cell_in_links(links1, cell0));

    // Test unlink functionality through lab namespace
    lab::unlink(cell0, cell1);
    REQUIRE_FALSE(cell0->is_linked(cell1));
    REQUIRE_FALSE(cell1->is_linked(cell0));
    
    // Verify links are removed
    links0 = cell0->get_links();
    links1 = cell1->get_links();
    
    REQUIRE_FALSE(find_cell_in_links(links0, cell1));
    REQUIRE_FALSE(find_cell_in_links(links1, cell0));
}

