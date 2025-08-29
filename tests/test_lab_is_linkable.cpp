#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/maze_builder.h>

using namespace mazes;
using namespace std;

TEST_CASE("Lab can link/unlink cells", "[links]") {
    
    SECTION("Bidirectional linking works correctly") {
        auto cell1 = std::make_shared<cell>(1);
        auto cell2 = std::make_shared<cell>(2);
        
        // Initially cells should not be linked
        REQUIRE_FALSE(cell1->is_linked(cell2));
        REQUIRE_FALSE(cell2->is_linked(cell1));
        
        // Link cells bidirectionally (default behavior)
        lab::link(cell1, cell2);
        
        // Both cells should now be linked to each other
        REQUIRE(cell1->is_linked(cell2));
        REQUIRE(cell2->is_linked(cell1));
    }
    
    SECTION("Unidirectional linking works correctly") {
        auto cell1 = std::make_shared<cell>(3);
        auto cell2 = std::make_shared<cell>(4);
        
        // Link unidirectionally
        lab::link(cell1, cell2, false);
        
        // Only cell1 should be linked to cell2
        REQUIRE(cell1->is_linked(cell2));
        REQUIRE_FALSE(cell2->is_linked(cell1));
    }
    
    SECTION("Unlinking works correctly") {
        auto cell1 = std::make_shared<cell>(5);
        auto cell2 = std::make_shared<cell>(6);
        
        // Link first
        lab::link(cell1, cell2);
        REQUIRE(cell1->is_linked(cell2));
        REQUIRE(cell2->is_linked(cell1));
        
        // Unlink bidirectionally
        lab::unlink(cell1, cell2);
        
        // No cells should be linked
        REQUIRE_FALSE(cell1->is_linked(cell2));
        REQUIRE_FALSE(cell2->is_linked(cell1));
    }
    
    SECTION("Unidirectional unlinking works correctly") {
        auto cell1 = std::make_shared<cell>(7);
        auto cell2 = std::make_shared<cell>(8);
        
        // Link bidirectionally first
        lab::link(cell1, cell2);
        
        // Unlink unidirectionally
        lab::unlink(cell1, cell2, false);
        
        // Only cell2 should still be linked to cell1
        REQUIRE_FALSE(cell1->is_linked(cell2));
        REQUIRE(cell2->is_linked(cell1));
    }
    
    SECTION("Null pointer handling in link") {
        auto cell1 = std::make_shared<cell>(9);
        
        // These should not crash
        lab::link(nullptr, cell1);
        lab::link(cell1, nullptr);
        lab::link(nullptr, nullptr);
        
        // Cell should still be in valid state
        REQUIRE(cell1->get_index() == 9);
    }
    
    SECTION("Null pointer handling in unlink") {
        auto cell1 = std::make_shared<cell>(10);
        
        // These should not crash
        lab::unlink(nullptr, cell1);
        lab::unlink(cell1, nullptr);
        lab::unlink(nullptr, nullptr);
        
        // Cell should still be in valid state
        REQUIRE(cell1->get_index() == 10);
    }
    
    SECTION("Multiple link/unlink operations") {
        auto cell1 = std::make_shared<cell>(11);
        auto cell2 = std::make_shared<cell>(12);
        auto cell3 = std::make_shared<cell>(13);
        
        // Link cell1 to both cell2 and cell3
        lab::link(cell1, cell2);
        lab::link(cell1, cell3);
        
        // Verify all links
        REQUIRE(cell1->is_linked(cell2));
        REQUIRE(cell1->is_linked(cell3));
        REQUIRE(cell2->is_linked(cell1));
        REQUIRE(cell3->is_linked(cell1));
        
        // Unlink one connection
        lab::unlink(cell1, cell2);
        
        // Verify only the correct link was removed
        REQUIRE_FALSE(cell1->is_linked(cell2));
        REQUIRE_FALSE(cell2->is_linked(cell1));
        REQUIRE(cell1->is_linked(cell3));
        REQUIRE(cell3->is_linked(cell1));
    }
    
    SECTION("Link same cell multiple times") {
        auto cell1 = std::make_shared<cell>(14);
        auto cell2 = std::make_shared<cell>(15);
        
        // Link the same cells multiple times
        lab::link(cell1, cell2);
        lab::link(cell1, cell2);
        lab::link(cell1, cell2);
        
        // Should still be linked correctly
        REQUIRE(cell1->is_linked(cell2));
        REQUIRE(cell2->is_linked(cell1));
        
        // One unlink should remove the connection
        lab::unlink(cell1, cell2);
        
        // Should be unlinked
        REQUIRE_FALSE(cell1->is_linked(cell2));
        REQUIRE_FALSE(cell2->is_linked(cell1));
    }
}

TEST_CASE("Lab set neighbors and verify links", "[neighbors]") {
    
    SECTION("Basic 2x2 grid neighbor setup") {
        configurator config;
        config.rows(2).columns(2).levels(1);
        
        std::vector<int> indices;
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        // Should have 4 cells (2x2x1)
        REQUIRE(cells.size() == 4);
        
        // Verify all cells exist and have correct indices
        for (size_t i = 0; i < cells.size(); ++i) {

            REQUIRE(cells.at(i) != nullptr);
            REQUIRE(cells.at(i)->get_index() == static_cast<int32_t>(i));
        }
        
        // Test specific neighbor relationships
        // Cell 0 (top-left) should be linked to cell 1 (right) and cell 2 (below)
        REQUIRE(cells[0]->is_linked(cells[1])); // East neighbor
        REQUIRE(cells[0]->is_linked(cells[2])); // South neighbor
        
        // Cell 1 (top-right) should be linked to cell 0 (left) and cell 3 (below)
        REQUIRE(cells[1]->is_linked(cells[0])); // West neighbor
        REQUIRE(cells[1]->is_linked(cells[3])); // South neighbor
        
        // Cell 2 (bottom-left) should be linked to cell 0 (above) and cell 3 (right)
        REQUIRE(cells[2]->is_linked(cells[0])); // North neighbor
        REQUIRE(cells[2]->is_linked(cells[3])); // East neighbor
        
        // Cell 3 (bottom-right) should be linked to cell 1 (above) and cell 2 (left)
        REQUIRE(cells[3]->is_linked(cells[1])); // North neighbor
        REQUIRE(cells[3]->is_linked(cells[2])); // West neighbor
    }
    
    SECTION("3x3 grid with specific neighbor counts") {
        configurator config;
        config.rows(3).columns(3).levels(1);
        
        std::vector<int> indices;
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        REQUIRE(cells.size() == 9);
        
        // Corner cells should have 2 neighbors each
        auto count_links = [](const std::shared_ptr<cell>& c) {
            auto links = c->get_links();
            return std::count_if(links.begin(), links.end(), 
                [](const auto& link) { return link.second; });
        };
        
        // Corners: 0, 2, 6, 8
        REQUIRE(count_links(cells[0]) == 2); // Top-left
        REQUIRE(count_links(cells[2]) == 2); // Top-right
        REQUIRE(count_links(cells[6]) == 2); // Bottom-left
        REQUIRE(count_links(cells[8]) == 2); // Bottom-right
        
        // Edge cells should have 3 neighbors each
        REQUIRE(count_links(cells[1]) == 3); // Top-middle
        REQUIRE(count_links(cells[3]) == 3); // Middle-left
        REQUIRE(count_links(cells[5]) == 3); // Middle-right
        REQUIRE(count_links(cells[7]) == 3); // Bottom-middle
        
        // Center cell should have 4 neighbors
        REQUIRE(count_links(cells[4]) == 4); // Center
    }
    
    SECTION("Reordering with valid indices") {
        configurator config;
        config.rows(2).columns(2).levels(1);
        
        // Provide reordering indices (reverse order)
        std::vector<int> indices = {3, 2, 1, 0};
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        REQUIRE(cells.size() == 4);
        
        // Verify the cells were reordered correctly
        // Original indices were 0,1,2,3 but reordered to 3,2,1,0
        REQUIRE(cells[0]->get_index() == 3);
        REQUIRE(cells[1]->get_index() == 2);
        REQUIRE(cells[2]->get_index() == 1);
        REQUIRE(cells[3]->get_index() == 0);
    }
    
    SECTION("Empty configuration is handled gracefully") {
        configurator config; // Default values
        std::vector<int> indices;
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        // Should create a 10x10x1 grid (default values)
        REQUIRE(cells.size() == 100);
        REQUIRE(cells[0] != nullptr);
        REQUIRE(cells[99] != nullptr);
    }
    
    SECTION("Single cell grid") {
        configurator config;
        config.rows(1).columns(1).levels(1);
        
        std::vector<int> indices;
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        REQUIRE(cells.size() == 1);
        REQUIRE(cells[0]->get_index() == 0);
        
        // Single cell should have no neighbors
        auto links = cells[0]->get_links();
        REQUIRE(links.empty());
    }
    
    SECTION("3D grid neighbor relationships") {
        configurator config;
        config.rows(2).columns(2).levels(2);
        
        std::vector<int> indices;
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        // Should have 8 cells (2x2x2)
        REQUIRE(cells.size() == 8);
        
        // Cells 0-3 are on level 0, cells 4-7 are on level 1
        // Each cell should have neighbors in same level (no inter-level connections in this implementation)
        
        // Cell 0 (level 0, row 0, col 0)
        REQUIRE(cells[0]->is_linked(cells[1])); // East
        REQUIRE(cells[0]->is_linked(cells[2])); // South
        
        // Cell 4 (level 1, row 0, col 0) 
        REQUIRE(cells[4]->is_linked(cells[5])); // East
        REQUIRE(cells[4]->is_linked(cells[6])); // South
    }
    
    SECTION("Linear grid (1xN) topology") {
        configurator config;
        config.rows(1).columns(5).levels(1); // 1x5 linear grid
        
        std::vector<int> indices;
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        REQUIRE(cells.size() == 5);
        
        // Check linear connectivity
        // Cell 0: only connected to cell 1 (east)
        REQUIRE(cells[0]->is_linked(cells[1]));
        auto links0 = cells[0]->get_links();
        REQUIRE(std::count_if(links0.begin(), links0.end(), 
            [](const auto& link) { return link.second; }) == 1);
        
        // Cell 2 (middle): connected to cells 1 and 3
        REQUIRE(cells[2]->is_linked(cells[1])); // West
        REQUIRE(cells[2]->is_linked(cells[3])); // East
        auto links2 = cells[2]->get_links();
        REQUIRE(std::count_if(links2.begin(), links2.end(), 
            [](const auto& link) { return link.second; }) == 2);
        
        // Cell 4 (end): only connected to cell 3 (west)
        REQUIRE(cells[4]->is_linked(cells[3]));
        auto links4 = cells[4]->get_links();
        REQUIRE(std::count_if(links4.begin(), links4.end(), 
            [](const auto& link) { return link.second; }) == 1);
    }
    
    SECTION("Vertical grid (Nx1) topology") {
        configurator config;
        config.rows(5).columns(1).levels(1); // 5x1 vertical grid
        
        std::vector<int> indices;
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        REQUIRE(cells.size() == 5);
        
        // Check vertical connectivity
        // Cell 0: only connected to cell 1 (south)
        REQUIRE(cells[0]->is_linked(cells[1]));
        auto links0 = cells[0]->get_links();
        REQUIRE(std::count_if(links0.begin(), links0.end(), 
            [](const auto& link) { return link.second; }) == 1);
        
        // Cell 2 (middle): connected to cells 1 and 3
        REQUIRE(cells[2]->is_linked(cells[1])); // North
        REQUIRE(cells[2]->is_linked(cells[3])); // South
        auto links2 = cells[2]->get_links();
        REQUIRE(std::count_if(links2.begin(), links2.end(), 
            [](const auto& link) { return link.second; }) == 2);
        
        // Cell 4 (end): only connected to cell 3 (north)
        REQUIRE(cells[4]->is_linked(cells[3]));
        auto links4 = cells[4]->get_links();
        REQUIRE(std::count_if(links4.begin(), links4.end(), 
            [](const auto& link) { return link.second; }) == 1);
    }
}

TEST_CASE("Lab error handling and edge cases", "[error handling]") {
    
    SECTION("Invalid configurator is handled gracefully") {
        // Create an invalid configurator by setting impossible values
        configurator config;
        // Note: The configurator class clamps values, so we test with an uninitialized one
        
        std::vector<int> indices;
        std::vector<std::shared_ptr<cell>> cells;
        
        // This should not crash and should handle the invalid config
        lab::set_neighbors(config, indices, cells);
        
        // Should either create default grid or empty grid
        // Based on implementation, should create default 10x10x1 = 100 cells
        REQUIRE(cells.size() == 100);
    }
    
    SECTION("Mismatched indices count") {
        configurator config;
        config.rows(2).columns(2).levels(1); // Expects 4 cells
        
        // Provide wrong number of indices
        std::vector<int> indices = {0, 1, 2}; // Only 3 indices for 4 cells
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        // Should handle gracefully and create cells anyway
        REQUIRE(cells.size() == 4);
    }
    
    SECTION("Out of bounds indices are handled") {
        configurator config;
        config.rows(2).columns(2).levels(1); // 4 cells total
        
        // Provide indices with out-of-bounds values
        std::vector<int> indices = {0, 1, 2, 10}; // 10 is out of bounds
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        // Should handle gracefully
        REQUIRE(cells.size() == 4);
    }
    
    SECTION("Negative indices are handled") {
        configurator config;
        config.rows(2).columns(2).levels(1);
        
        std::vector<int> indices = {0, 1, 2, -1}; // Negative index
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        // Should handle gracefully
        REQUIRE(cells.size() == 4);
    }
    
    SECTION("Very large grid dimensions") {
        configurator config;
        config.rows(3).columns(3).levels(3); // Still reasonable size for testing
        
        std::vector<int> indices;
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        REQUIRE(cells.size() == 27); // 3x3x3 = 27 cells
        
        // Verify all cells are created properly
        for (const auto& cell : cells) {
            REQUIRE(cell != nullptr);
        }
    }
    
    SECTION("Duplicate indices in reordering") {
        configurator config;
        config.rows(2).columns(2).levels(1); // 4 cells total
        
        // Provide indices with duplicates
        std::vector<int> indices = {0, 1, 1, 2}; // Duplicate index 1
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        // Should handle gracefully - either use original order or apply partial reordering
        REQUIRE(cells.size() == 4);
        
        // All cells should still be valid
        for (const auto& cell : cells) {
            REQUIRE(cell != nullptr);
        }
    }
    
    SECTION("Empty grid (minimum dimensions)") {
        configurator config;
        config.rows(1).columns(1).levels(1);
        
        std::vector<int> indices;
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        REQUIRE(cells.size() == 1);
        REQUIRE(cells[0] != nullptr);
        REQUIRE(cells[0]->get_index() == 0);
        
        // Single cell should have no links
        auto links = cells[0]->get_links();
        REQUIRE(links.empty());
    }
    
    SECTION("Stress test with moderate sized grid") {
        configurator config;
        config.rows(10).columns(10).levels(2); // 200 cells total
        
        std::vector<int> indices;
        std::vector<std::shared_ptr<cell>> cells;
        
        lab::set_neighbors(config, indices, cells);
        
        REQUIRE(cells.size() == 200);
        
        // Verify all cells exist and have valid indices
        for (size_t i = 0; i < cells.size(); ++i) {
            REQUIRE(cells[i] != nullptr);
            REQUIRE(cells[i]->get_index() == static_cast<int32_t>(i));
        }
        
        // Verify some cells have the expected number of neighbors
        // Corner cell of first level (index 0) should have 2 neighbors
        auto links_corner = cells[0]->get_links();
        REQUIRE(std::count_if(links_corner.begin(), links_corner.end(), 
            [](const auto& link) { return link.second; }) == 2);
        
        // Middle cell of first level should have 4 neighbors
        size_t middle_index = 5 * 10 + 5; // Row 5, Column 5, Level 0
        auto links_middle = cells[middle_index]->get_links();
        REQUIRE(std::count_if(links_middle.begin(), links_middle.end(), 
            [](const auto& link) { return link.second; }) == 4);
    }
}
