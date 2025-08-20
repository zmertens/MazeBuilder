#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <list>
#include <memory>
#include <random>
#include <type_traits>

#include <MazeBuilder/maze_builder.h>
#include <MazeBuilder/maze_adapter.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/stringify.h>

using namespace mazes;
using namespace std;

/*
TEST_CASE("string_view_utils::split works correctly", "[string_view_utils]") {

    SECTION("Split with bracket delimiter") {

        std::string_view example = "-d[0:-1]";
        auto result = string_view_utils::split(example, "[");
        
        REQUIRE(result.size() == 2);
        auto it = result.begin();
        REQUIRE(*it == "-d");
        ++it;
        REQUIRE(*it == "0:-1]");
    }
    
    SECTION("Split with default space delimiter") {

        std::string_view example = "hello world test";
        auto result = string_view_utils::split(example);
        
        REQUIRE(result.size() == 3);
        auto it = result.begin();
        REQUIRE(*it == "hello");
        ++it;
        REQUIRE(*it == "world");
        ++it;
        REQUIRE(*it == "test");
    }
    
    SECTION("Split with custom delimiter") {
        std::string_view example = "one,two,three";
        auto result = string_view_utils::split(example, ",");
        
        REQUIRE(result.size() == 3);
        auto it = result.begin();
        REQUIRE(*it == "one");
        ++it;
        REQUIRE(*it == "two");
        ++it;
        REQUIRE(*it == "three");
    }
    
    SECTION("Split with multi-character delimiter") {
        std::string_view example = "one::two::three";
        auto result = string_view_utils::split(example, "::");
        
        REQUIRE(result.size() == 3);
        auto it = result.begin();
        REQUIRE(*it == "one");
        ++it;
        REQUIRE(*it == "two");
        ++it;
        REQUIRE(*it == "three");
    }
    
    SECTION("Split empty string") {
        std::string_view example = "";
        auto result = string_view_utils::split(example, ",");
        
        REQUIRE(result.empty());
    }
    
    SECTION("Split with no delimiter found") {
        std::string_view example = "nodlimiterhere";
        auto result = string_view_utils::split(example, ",");
        
        REQUIRE(result.size() == 1);
        REQUIRE(*result.begin() == "nodlimiterhere");
    }
}

TEST_CASE("maze_adapter integration with stringify", "[maze_adapter stringify]") {
    
    SECTION("Test maze_adapter with stringify operations") {
        // Create a small grid for testing
        grid_factory factory;
        configurator config;
        config.rows(3).columns(3).levels(1).seed(12345);
        
        auto grid_ptr = factory.create(config);
        REQUIRE(grid_ptr != nullptr);
        
        // Get cells using maze_adapter
        auto cells = grid_ptr->operations().get_cells();
        maze_adapter maze_view(cells);
        
        REQUIRE_FALSE(maze_view.empty());
        REQUIRE(maze_view.size() == 9); // 3x3 grid
        
        // Test that we can find specific cells
        auto corner_cell = maze_view.find(0); // Top-left corner
        REQUIRE(corner_cell != maze_view.end());
        REQUIRE((*corner_cell)->get_index() == 0);
        
        auto center_cell = maze_view.find(4); // Center cell
        REQUIRE(center_cell != maze_view.end());
        REQUIRE((*center_cell)->get_index() == 4);
        
        // Test range operations for row-based access
        auto first_row = maze_view.cbegin(0, 3); // First row: indices 0, 1, 2
        REQUIRE(first_row != maze_view.end());
        REQUIRE((*first_row)->get_index() == 0);
        
        // Verify we can iterate through the first row
        int expected_index = 0;
        for (auto it = first_row; it != first_row + 3; ++it) {
            REQUIRE((*it)->get_index() == expected_index);
            ++expected_index;
        }
    }
    
    SECTION("Test stringify with maze_adapter integration") {
        // Create a grid and run stringify
        grid_factory factory;
        configurator config;
        config.rows(2).columns(2).levels(1).seed(54321);
        
        auto grid_ptr = factory.create(config);
        REQUIRE(grid_ptr != nullptr);
        
        // Run stringify which now uses maze_adapter internally
        randomizer rng;
        rng.seed(54321);
        
        stringify str_processor;
        bool success = str_processor.run(grid_ptr, rng);
        REQUIRE(success);
        
        // Verify we got a string output
        std::string result = grid_ptr->operations().get_str();
        REQUIRE_FALSE(result.empty());
        
        // Basic validation - should contain maze characters
        REQUIRE(result.find('+') != std::string::npos); // Should have corners
        REQUIRE(result.find('|') != std::string::npos); // Should have walls
        REQUIRE(result.find('-') != std::string::npos); // Should have horizontal walls
    }
}

TEST_CASE("maze_adapter performance with large grids", "[maze_adapter performance]") {
    
    SECTION("Performance test with 10x10 grid") {
        // Create a larger grid to test performance
        grid_factory factory;
        configurator config;
        config.rows(10).columns(10).levels(1).seed(99999);
        
        auto grid_ptr = factory.create(config);
        REQUIRE(grid_ptr != nullptr);
        
        auto cells = grid_ptr->operations().get_cells();
        maze_adapter maze_view(cells);
        
        REQUIRE(maze_view.size() == 100);
        
        // Test various maze_adapter operations for performance
        
        // Test search operations
        auto found_cell = maze_view.find(50); // Middle-ish cell
        REQUIRE(found_cell != maze_view.end());
        
        // Test range operations
        auto row_5 = maze_view.cbegin(50, 10); // Row 5 (indices 50-59)
        REQUIRE(row_5 != maze_view.end());
        
        // Test filtering operations
        auto even_cells = maze_view.count_if([](const std::shared_ptr<cell>& c) {
            return c && (c->get_index() % 2 == 0);
        });
        REQUIRE(even_cells == 50); // Half the cells should have even indices
        
        // Test sorting
        auto sorted_view = maze_view.sort_by_index();
        REQUIRE(sorted_view.size() == 100);
        REQUIRE(sorted_view.front()->get_index() <= sorted_view.back()->get_index());
    }
}

TEST_CASE("maze_adapter string_view-like operations", "[maze_adapter string_view]") {
    
    SECTION("Test substr operations") {
        // Create test cells
        std::vector<std::shared_ptr<cell>> test_cells;
        for (int i = 0; i < 20; ++i) {
            test_cells.push_back(std::make_shared<cell>(i));
        }
        
        maze_adapter adapter(test_cells);
        
        // Test substr(pos)
        auto sub1 = adapter.substr(5);
        REQUIRE(sub1.size() == 15);
        REQUIRE(sub1.front()->get_index() == 5);
        REQUIRE(sub1.back()->get_index() == 19);
        
        // Test substr(pos, len)
        auto sub2 = adapter.substr(8, 5);
        REQUIRE(sub2.size() == 5);
        REQUIRE(sub2.front()->get_index() == 8);
        REQUIRE(sub2.back()->get_index() == 12);
        
        // Test contains operation
        REQUIRE(adapter.contains(10));
        REQUIRE_FALSE(adapter.contains(25));
        
        // Test count operation
        REQUIRE(adapter.count(15) == 1);
        REQUIRE(adapter.count(100) == 0);
    }
    
    SECTION("Test maze_adapter with null cells") {
        std::vector<std::shared_ptr<cell>> mixed_cells;
        mixed_cells.push_back(std::make_shared<cell>(0));
        mixed_cells.push_back(nullptr);
        mixed_cells.push_back(std::make_shared<cell>(2));
        mixed_cells.push_back(nullptr);
        mixed_cells.push_back(std::make_shared<cell>(4));
        
        maze_adapter adapter(mixed_cells);
        REQUIRE(adapter.size() == 5);
        
        // Test remove_nulls
        auto filtered = adapter.remove_nulls();
        REQUIRE(filtered.size() == 3);
        
        // Verify remaining cells
        auto indices = filtered.get_indices();
        REQUIRE(indices.size() == 3);
        REQUIRE(std::find(indices.begin(), indices.end(), 0) != indices.end());
        REQUIRE(std::find(indices.begin(), indices.end(), 2) != indices.end());
        REQUIRE(std::find(indices.begin(), indices.end(), 4) != indices.end());
    }
}

TEST_CASE( "Benchmark stringz ops ", "[benchmark stringz]" ) {

    static constexpr auto ROWS = 50, COLUMNS = 50, LEVELS = 10;
    static constexpr auto SEED = 12345;
    static constexpr auto ALGO = algo::DFS;

    randomizer rng;
    rng.seed(SEED);

    // Test with maze_adapter integration
    SECTION("Benchmark with maze_adapter") {
        grid_factory factory;
        configurator config;
        config.rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO).seed(SEED);
        
        auto grid_ptr = factory.create(config);
        REQUIRE(grid_ptr != nullptr);
        
        // Get maze adapter
        auto cells = grid_ptr->operations().get_cells();
        maze_adapter maze_view(cells);
        
        REQUIRE(maze_view.size() == ROWS * COLUMNS * LEVELS);
        
        // Test various operations
        auto found_cells = maze_view.count_if([](const std::shared_ptr<cell>& c) {
            return c != nullptr;
        });
        
        REQUIRE(found_cells > 0);
    }

    BENCHMARK("Benchmark stringz::stringify") {
        grid_factory factory;
        configurator config;
        config.rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO).seed(SEED);

        auto grid_ptr = factory.create(config);
        REQUIRE(grid_ptr != nullptr);
        stringify str_processor;
        REQUIRE(str_processor.run(ref(grid_ptr), ref(rng)));
    };
}
*/

