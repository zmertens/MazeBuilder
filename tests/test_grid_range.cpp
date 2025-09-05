#include <MazeBuilder/grid.h>
#include <MazeBuilder/cell.h>
#include <iostream>
#include <vector>

using namespace mazes;

int main()
{
    try {
        // Create a 3x3 grid
        grid g(3, 3, 1);

        std::cout << "Testing grid_range functionality...\n\n";

        // Test 1: Basic range iteration
        std::cout << "Test 1: Iterating over all cells (should create 9 cells)\n";
        int count = 0;
        for (const auto& cell_ptr : g.cells())
        {
            if (cell_ptr)
            {
                std::cout << "Cell " << cell_ptr->get_index() << " ";
                count++;
            }
        }
        std::cout << "\nCreated " << count << " cells\n\n";

        // Test 2: Partial range iteration
        std::cout << "Test 2: Iterating over cells [0, 5)\n";
        for (const auto& cell_ptr : g.cells(0, 5))
        {
            if (cell_ptr)
            {
                std::cout << "Cell " << cell_ptr->get_index() << " ";
            }
        }
        std::cout << "\n\n";

        // Test 3: Using simplified get_cells
        std::cout << "Test 3: Using simplified get_cells() method\n";
        auto all_cells = g.get_cells();
        std::cout << "get_cells() returned " << all_cells.size() << " cells\n\n";

        // Test 4: Using range to_vector
        std::cout << "Test 4: Using cells().to_vector()\n";
        auto range_cells = g.cells().to_vector();
        std::cout << "cells().to_vector() returned " << range_cells.size() << " cells\n\n";

        // Test 5: Clear a subset and check
        std::cout << "Test 5: Clearing cells [0, 3)\n";
        g.cells(0, 3).clear();
        std::cout << "After clearing [0, 3), total cells: " << g.num_cells() << "\n\n";

        // Test 6: Simplified set_cells
        std::cout << "Test 6: Using simplified set_cells\n";
        std::vector<std::shared_ptr<cell>> new_cells;
        for (int i = 0; i < 6; ++i)
        {
            new_cells.push_back(std::make_shared<cell>(i));
        }
        g.set_cells(new_cells);
        std::cout << "After set_cells with 6 cells, total: " << g.num_cells() << "\n\n";

        // Test 7: Simplified clear_cells
        std::cout << "Test 7: Using simplified clear_cells\n";
        g.clear_cells();
        std::cout << "After clear_cells(), total: " << g.num_cells() << "\n\n";

        std::cout << "All tests completed successfully!\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
