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
#include <MazeBuilder/maze_adapter.h>
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

// New maze_adapter tests integrated into test_grid.cpp
TEST_CASE("maze_adapter with grid integration", "[maze_adapter grid]") {

    SECTION("maze_adapter with grid cells") {
        // Get cells from a real grid
        auto cells = my_grid->operations().get_cells();
        maze_adapter maze_view(cells);

        REQUIRE(maze_view.size() == cells.size());

        if (!maze_view.empty()) {
            // Test basic iteration functionality
            auto beg1 = maze_view.cbegin(0, std::min(static_cast<size_t>(10), maze_view.size()));
            REQUIRE(beg1 != maze_view.end());

            auto beg2 = maze_view.cend();
            REQUIRE(beg2 == maze_view.end());

            auto beg3 = maze_view.find(-1);
            REQUIRE(beg3 == maze_view.end()); // Should not find invalid index
        }
    }

    SECTION("maze_adapter iteration methods validation") {
        // Create a small test grid for validation
        std::vector<std::shared_ptr<cell>> test_cells;
        for (int i = 0; i < 25; ++i) { // 5x5 grid
            test_cells.push_back(std::make_shared<cell>(i));
        }

        maze_adapter adapter(test_cells);

        // Validate the specific API usage patterns mentioned
        SECTION("cbegin(0, 10) usage pattern") {
            auto beg1 = adapter.cbegin(0, 10);
            REQUIRE((*beg1)->get_index() == 0);

            // Verify we can iterate through the range
            int count = 0;
            for (auto it = beg1; it != beg1 + 10; ++it) {
                REQUIRE((*it)->get_index() == count);
                ++count;
            }
            REQUIRE(count == 10);
        }

        SECTION("cend() usage pattern") {
            auto beg2 = adapter.cend();
            REQUIRE(beg2 == adapter.end());
        }

        SECTION("find(-1) usage pattern") {
            auto beg3 = adapter.find(-1);
            REQUIRE(beg3 == adapter.end());
        }

        SECTION("Additional string_view-like operations") {
            // Test substr functionality
            auto sub_view = adapter.substr(5, 10);
            REQUIRE(sub_view.size() == 10);
            REQUIRE(sub_view.front()->get_index() == 5);
            REQUIRE(sub_view.back()->get_index() == 14);

            // Test contains functionality
            REQUIRE(adapter.contains(12));
            REQUIRE_FALSE(adapter.contains(100));

            // Test count functionality
            REQUIRE(adapter.count(15) == 1);
            REQUIRE(adapter.count(-1) == 0);
        }
    }

    SECTION("maze printing simulation") {
        // Simulate the 10x10 maze printing scenario
        std::vector<std::shared_ptr<cell>> maze_cells;
        for (int i = 0; i < 100; ++i) { // 10x10 = 100 cells
            maze_cells.push_back(std::make_shared<cell>(i));
        }

        maze_adapter maze_view(maze_cells);

        // Simulate iterating through the maze for printing
        REQUIRE(maze_view.size() == 100);

        // Test row-by-row access (as would be needed for printing)
        for (int row = 0; row < 10; ++row) {
            auto row_start = maze_view.cbegin(row * 10, 10);
            auto row_end = row_start + 10;

            int expected_index = row * 10;
            for (auto it = row_start; it != row_end; ++it) {
                REQUIRE((*it)->get_index() == expected_index);
                ++expected_index;
            }
        }

        // Test finding specific cells (corners, center, etc.)
        REQUIRE(maze_view.find(0) != maze_view.end());   // Top-left
        REQUIRE(maze_view.find(9) != maze_view.end());   // Top-right
        REQUIRE(maze_view.find(90) != maze_view.end());  // Bottom-left
        REQUIRE(maze_view.find(99) != maze_view.end());  // Bottom-right
        REQUIRE(maze_view.find(44) != maze_view.end());  // Center-ish
    }
}
