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
bool find_cell_in_links(const std::vector<std::pair<std::shared_ptr<cell>, bool>> &links, const std::shared_ptr<cell> &target)
{
    return std::any_of(links.begin(), links.end(), [&target](const auto &link)
                       { return link.first == target; });
}

TEST_CASE("Static Assert grid", "[grid static asserts]")
{
    STATIC_REQUIRE(std::is_default_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::grid>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::grid>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::grid>::value);

    STATIC_REQUIRE(std::is_default_constructible<mazes::cell>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::cell>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::cell>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::cell>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::cell>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::cell>::value);
}

TEST_CASE("Test grid dimensions", "[grid dimensions]")
{

    SECTION(" Regular grid ")
    {
        auto [rows, columns, height] = my_grid_distances->operations().get_dimensions();

        REQUIRE(rows == ROWS);
        REQUIRE(columns == COLUMNS);
        REQUIRE(height == HEIGHT);
    }

    SECTION(" Distance grid ")
    {
        auto [rows, columns, height] = my_grid_distances->operations().get_dimensions();

        REQUIRE(rows == ROWS);
        REQUIRE(columns == COLUMNS);
        REQUIRE(height == HEIGHT);
    }

    SECTION(" Colored grid ")
    {
        auto [rows, columns, height] = my_grid_distances->operations().get_dimensions();

        REQUIRE(rows == ROWS);
        REQUIRE(columns == COLUMNS);
        REQUIRE(height == HEIGHT);
    }
}

TEST_CASE("Grid Range Basic Functionality", "[grid_range]")
{
    grid test_grid(3, 3, 1); // 3x3 grid = 9 cells total

    SECTION("Range iteration creates cells lazily")
    {
        // Initially no cells should be created
        REQUIRE(test_grid.num_cells() == 0);
        
        // Iterate through range should create cells on demand
        int count = 0;
        for (const auto& cell_ptr : test_grid.cells())
        {
            REQUIRE(cell_ptr != nullptr);
            REQUIRE(cell_ptr->get_index() == count);
            count++;
        }
        
        REQUIRE(count == 9); // 3x3 = 9 cells
        REQUIRE(test_grid.num_cells() == 9);
    }

    SECTION("Subset range iteration")
    {
        int count = 0;
        for (const auto& cell_ptr : test_grid.cells(2, 5))
        {
            REQUIRE(cell_ptr != nullptr);
            REQUIRE(cell_ptr->get_index() >= 2);
            REQUIRE(cell_ptr->get_index() < 5);
            count++;
        }
        
        REQUIRE(count == 3); // indices 2, 3, 4
    }

    SECTION("Range to_vector conversion")
    {
        // First, force creation of cells by iterating
        for (const auto& cell_ptr : test_grid.cells())
        {
            (void)cell_ptr; // Force creation
        }
        
        auto cells_vector = test_grid.cells().to_vector();
        REQUIRE(cells_vector.size() == 9);
        
        // Check that cells are in order
        for (size_t i = 0; i < cells_vector.size(); ++i)
        {
            REQUIRE(cells_vector[i] != nullptr);
            REQUIRE(cells_vector[i]->get_index() == static_cast<int>(i));
        }
    }

    SECTION("Range size and empty checks")
    {
        auto full_range = test_grid.cells();
        REQUIRE(full_range.size() == 9);
        REQUIRE_FALSE(full_range.empty());
        
        auto subset_range = test_grid.cells(1, 4);
        REQUIRE(subset_range.size() == 3);
        REQUIRE_FALSE(subset_range.empty());
        
        auto empty_range = test_grid.cells(5, 5);
        REQUIRE(empty_range.size() == 0);
        REQUIRE(empty_range.empty());
    }
}

TEST_CASE("Grid Range Operations", "[grid_range_ops]")
{
    grid test_grid(4, 4, 1); // 4x4 grid = 16 cells total

    SECTION("Clear range functionality")
    {
        // Create some cells first
        for (const auto& cell_ptr : test_grid.cells(0, 8))
        {
            (void)cell_ptr; // Force creation
        }
        REQUIRE(test_grid.num_cells() == 8);
        
        // Clear a subset
        test_grid.cells(2, 6).clear();
        REQUIRE(test_grid.num_cells() == 4); // 8 - 4 = 4 remaining
        
        // Verify the right cells were cleared by checking existing cells
        std::vector<int> actual_indices;
        for (int i = 0; i < 16; ++i) // Check all possible indices in 4x4 grid
        {
            auto cell_ptr = test_grid.search(i); // Use search instead of range iteration
            if (cell_ptr)
            {
                actual_indices.push_back(cell_ptr->get_index());
            }
        }
        
        std::sort(actual_indices.begin(), actual_indices.end());
        std::vector<int> expected_indices = {0, 1, 6, 7};
        REQUIRE(actual_indices == expected_indices);
    }

    SECTION("Set from vector functionality")
    {
        // Create some test cells
        std::vector<std::shared_ptr<cell>> test_cells;
        for (int i = 0; i < 6; ++i)
        {
            test_cells.push_back(std::make_shared<cell>(i));
        }
        
        // Set cells using range - this should clear all cells first, then set new ones
        test_grid.set_cells(test_cells);
        REQUIRE(test_grid.num_cells() == 6);
        
        // Verify cells were set correctly
        auto cells_vector = test_grid.get_cells();
        REQUIRE(cells_vector.size() == 6);
        
        for (size_t i = 0; i < 6; ++i)
        {
            REQUIRE(cells_vector[i] != nullptr);
            REQUIRE(cells_vector[i]->get_index() == static_cast<int>(i));
        }
    }
}

TEST_CASE("Grid Range Iterator Behavior", "[grid_range_iterator]")
{
    grid test_grid(2, 3, 1); // 2x3 grid = 6 cells total

    SECTION("Iterator increment and dereference")
    {
        auto range = test_grid.cells();
        auto it = range.begin();
        auto end_it = range.end();
        
        REQUIRE(it != end_it);
        
        int expected_index = 0;
        while (it != end_it)
        {
            auto cell_ptr = *it;
            REQUIRE(cell_ptr != nullptr);
            REQUIRE(cell_ptr->get_index() == expected_index);
            
            ++it;
            ++expected_index;
        }
        
        REQUIRE(expected_index == 6);
    }

    SECTION("Iterator bounds checking")
    {
        // Out of bounds range
        auto range = test_grid.cells(10, 20);
        
        int count = 0;
        for (const auto& cell_ptr : range)
        {
            (void)cell_ptr;
            count++;
        }
        
        // Should not create any cells since indices are out of bounds
        REQUIRE(count == 0);
        
        // Additional edge cases to prevent infinite loops
        auto range2 = test_grid.cells(100, 200); // Way out of bounds
        int count2 = 0;
        for (const auto& cell_ptr : range2)
        {
            (void)cell_ptr;
            count2++;
        }
        REQUIRE(count2 == 0);
        
        // Start index at boundary
        auto range3 = test_grid.cells(6, 10); // Start at max_valid_index
        int count3 = 0;
        for (const auto& cell_ptr : range3)
        {
            (void)cell_ptr;
            count3++;
        }
        REQUIRE(count3 == 0);
        
        // Verify range properties for out of bounds
        REQUIRE(range.empty());
        REQUIRE(range.size() == 0);
        REQUIRE(range2.empty());
        REQUIRE(range2.size() == 0);
        REQUIRE(range3.empty());
        REQUIRE(range3.size() == 0);
    }

    SECTION("Post-increment iterator")
    {
        auto range = test_grid.cells(0, 3);
        auto it = range.begin();
        
        auto cell1 = *it;
        auto old_it = it++;
        auto cell2 = *it;
        
        REQUIRE(cell1 != nullptr);
        REQUIRE(cell2 != nullptr);
        REQUIRE(cell1->get_index() == 0);
        REQUIRE(cell2->get_index() == 1);
        REQUIRE(*old_it == cell1);
    }
}

TEST_CASE("Grid Range Const Correctness", "[grid_range_const]")
{
    grid test_grid(3, 2, 1); // 3x2 grid = 6 cells total
    
    // Create some cells first
    for (const auto& cell_ptr : test_grid.cells())
    {
        (void)cell_ptr; // Force creation
    }

    SECTION("Const range access")
    {
        const grid& const_grid = test_grid;
        
        int count = 0;
        for (const auto& cell_ptr : const_grid.cells())
        {
            REQUIRE(cell_ptr != nullptr);
            count++;
        }
        
        REQUIRE(count == 6);
    }

    SECTION("Const range subset access")
    {
        const grid& const_grid = test_grid;
        
        int count = 0;
        for (const auto& cell_ptr : const_grid.cells(1, 4))
        {
            REQUIRE(cell_ptr != nullptr);
            REQUIRE(cell_ptr->get_index() >= 1);
            REQUIRE(cell_ptr->get_index() < 4);
            count++;
        }
        
        REQUIRE(count == 3);
    }
}

TEST_CASE("Grid Range Integration with Simplified Methods", "[grid_range_integration]")
{
    grid test_grid(3, 3, 1);

    SECTION("Simplified get_cells using range")
    {
        // Create some cells using range iteration
        for (const auto& cell_ptr : test_grid.cells(0, 5))
        {
            (void)cell_ptr; // Force creation
        }
        
        // Test simplified get_cells method
        auto cells_vector = test_grid.get_cells();
        REQUIRE(cells_vector.size() == 5);
        
        // Verify order is maintained
        for (size_t i = 0; i < cells_vector.size(); ++i)
        {
            REQUIRE(cells_vector[i] != nullptr);
            REQUIRE(cells_vector[i]->get_index() == static_cast<int>(i));
        }
    }

    SECTION("Simplified set_cells using range")
    {
        std::vector<std::shared_ptr<cell>> new_cells;
        for (int i = 0; i < 4; ++i)
        {
            new_cells.push_back(std::make_shared<cell>(i * 2)); // indices 0, 2, 4, 6
        }
        
        // Test simplified set_cells method
        REQUIRE(test_grid.set_cells(new_cells));
        REQUIRE(test_grid.num_cells() == 4);
        
        // Verify cells were set correctly
        auto result_cells = test_grid.get_cells();
        REQUIRE(result_cells.size() == 4);
        
        std::vector<int> indices;
        for (const auto& cell_ptr : result_cells)
        {
            indices.push_back(cell_ptr->get_index());
        }
        std::sort(indices.begin(), indices.end());
        
        std::vector<int> expected = {0, 2, 4, 6};
        REQUIRE(indices == expected);
    }

    SECTION("Simplified clear_cells using range")
    {
        // Create some cells
        for (const auto& cell_ptr : test_grid.cells(0, 7))
        {
            (void)cell_ptr; // Force creation
        }
        REQUIRE(test_grid.num_cells() == 7);
        
        // Test simplified clear_cells method
        test_grid.clear_cells();
        REQUIRE(test_grid.num_cells() == 0);
        
        // Verify no cells remain
        auto cells_vector = test_grid.get_cells();
        REQUIRE(cells_vector.empty());
    }
}

TEST_CASE("Grid Range Edge Cases", "[grid_range_edge]")
{
    SECTION("Empty grid range")
    {
        grid empty_grid(0, 0, 0);
        
        int count = 0;
        for (const auto& cell_ptr : empty_grid.cells())
        {
            (void)cell_ptr;
            count++;
        }
        REQUIRE(count == 0);
        
        auto range = empty_grid.cells();
        REQUIRE(range.empty());
        REQUIRE(range.size() == 0);
    }

    SECTION("Single cell grid")
    {
        grid single_grid(1, 1, 1);
        
        int count = 0;
        for (const auto& cell_ptr : single_grid.cells())
        {
            REQUIRE(cell_ptr != nullptr);
            REQUIRE(cell_ptr->get_index() == 0);
            count++;
        }
        REQUIRE(count == 1);
    }

    SECTION("Invalid range bounds")
    {
        grid test_grid(3, 3, 1);
        
        // Negative start index should be clamped to 0
        auto range1 = test_grid.cells(-5, 3);
        REQUIRE(range1.size() == 3);
        
        // End index beyond bounds should be clamped
        auto range2 = test_grid.cells(5, 20);
        REQUIRE(range2.size() == 4); // indices 5, 6, 7, 8 (if they exist)
        
        // Reversed range (start > end) should be empty
        auto range3 = test_grid.cells(7, 3);
        REQUIRE(range3.empty());
    }
}
