#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <MazeBuilder/args.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/hexagonal_grid.h>
#include <MazeBuilder/randomizer.h>

using namespace mazes;
using namespace std;

static constexpr auto HEX_ROWS = 5, HEX_COLS = 5, HEX_LEVELS = 1;

TEST_CASE("Hexagonal grid static assertions", "[hexagonal_grid_static]")
{
    STATIC_REQUIRE(std::is_default_constructible<mazes::hexagonal_grid>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::hexagonal_grid>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::hexagonal_grid>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::hexagonal_grid>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::hexagonal_grid>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::hexagonal_grid>::value);
}

TEST_CASE("Hexagonal grid creation", "[hexagonal_grid_creation]")
{
    SECTION("Default construction")
    {
        hexagonal_grid hg{};
        auto [rows, cols, levels] = hg.operations().get_dimensions();
        REQUIRE(rows == 1u);
        REQUIRE(cols == 1u);
        REQUIRE(levels == 1u);
        REQUIRE(hg.num_cells() == 1);
    }

    SECTION("Parameterized construction")
    {
        hexagonal_grid hg{HEX_ROWS, HEX_COLS, HEX_LEVELS};
        auto [rows, cols, levels] = hg.operations().get_dimensions();
        REQUIRE(rows == static_cast<unsigned>(HEX_ROWS));
        REQUIRE(cols == static_cast<unsigned>(HEX_COLS));
        REQUIRE(levels == static_cast<unsigned>(HEX_LEVELS));
        REQUIRE(hg.num_cells() == HEX_ROWS * HEX_COLS * HEX_LEVELS);
    }

    SECTION("Tuple construction")
    {
        hexagonal_grid hg{std::make_tuple(3u, 4u, 1u)};
        auto [rows, cols, levels] = hg.operations().get_dimensions();
        REQUIRE(rows == 3u);
        REQUIRE(cols == 4u);
        REQUIRE(levels == 1u);
    }
}

TEST_CASE("Hexagonal grid 6-way connectivity", "[hexagonal_grid_neighbors]")
{
    hexagonal_grid hg{HEX_ROWS, HEX_COLS, HEX_LEVELS};

    SECTION("Center cell has up to 6 neighbors")
    {
        // Cell at row=2, col=2 (center of a 5x5 grid, index = 2*5+2 = 12)
        auto center = hg.search(12);
        REQUIRE(center != nullptr);

        auto neighbors = hg.get_neighbors(center);
        // Interior cell should have 6 neighbors
        REQUIRE(neighbors.size() == 6u);
    }

    SECTION("Corner cell has fewer neighbors")
    {
        // Cell at row=0, col=0 (top-left corner, index=0)
        auto corner = hg.search(0);
        REQUIRE(corner != nullptr);

        auto neighbors = hg.get_neighbors(corner);
        // Corner cell should have fewer than 6 neighbors
        REQUIRE(neighbors.size() < 6u);
        REQUIRE(neighbors.size() >= 1u);
    }

    SECTION("No NORTH/SOUTH neighbors in pointy-top hex")
    {
        auto cell_ptr = hg.search(12);
        REQUIRE(cell_ptr != nullptr);

        // NORTH and SOUTH should return nullptr in hexagonal topology
        REQUIRE(hg.get_north(cell_ptr) == nullptr);
        REQUIRE(hg.get_south(cell_ptr) == nullptr);
    }

    SECTION("EAST/WEST neighbors exist for interior cells")
    {
        // Cell at row=2, col=2 (index=12) should have EAST (col=3, index=13) and WEST (col=1, index=11)
        auto cell_ptr = hg.search(12);
        REQUIRE(cell_ptr != nullptr);

        auto east = hg.get_east(cell_ptr);
        auto west = hg.get_west(cell_ptr);
        REQUIRE(east != nullptr);
        REQUIRE(west != nullptr);
        REQUIRE(east->get_index() == 13);
        REQUIRE(west->get_index() == 11);
    }

    SECTION("Diagonal neighbors use even-r offset for even rows")
    {
        // Cell at row=0 (even), col=2 (index=2)
        // Even row: NE -> (row-1, col) out of bounds, NW -> (row-1, col-1) out of bounds
        // SE -> (row+1, col=2) = index=7, SW -> (row+1, col-1=1) = index=6
        auto cell_ptr = hg.search(2); // row=0, col=2
        REQUIRE(cell_ptr != nullptr);

        auto se = hg.get_southeast(cell_ptr);
        auto sw = hg.get_southwest(cell_ptr);
        REQUIRE(se != nullptr);
        REQUIRE(sw != nullptr);
        REQUIRE(se->get_index() == 7);  // (row=1, col=2)
        REQUIRE(sw->get_index() == 6);  // (row=1, col=1)
    }

    SECTION("Diagonal neighbors use even-r offset for odd rows")
    {
        // Cell at row=1 (odd), col=2 (index=7)
        // Odd row: NE -> (row-1, col+1=3) = index=3, NW -> (row-1, col=2) = index=2
        // SE -> (row+1, col+1=3) = index=13, SW -> (row+1, col=2) = index=12
        auto cell_ptr = hg.search(7); // row=1, col=2
        REQUIRE(cell_ptr != nullptr);

        auto ne = hg.get_northeast(cell_ptr);
        auto nw = hg.get_northwest(cell_ptr);
        auto se = hg.get_southeast(cell_ptr);
        auto sw = hg.get_southwest(cell_ptr);
        REQUIRE(ne != nullptr);
        REQUIRE(nw != nullptr);
        REQUIRE(se != nullptr);
        REQUIRE(sw != nullptr);
        REQUIRE(ne->get_index() == 3);   // (row=0, col=3)
        REQUIRE(nw->get_index() == 2);   // (row=0, col=2)
        REQUIRE(se->get_index() == 13);  // (row=2, col=3)
        REQUIRE(sw->get_index() == 12);  // (row=2, col=2)
    }

    SECTION("get_neighbor matches convenience methods")
    {
        auto cell_ptr = hg.search(7); // row=1 (odd), col=2
        REQUIRE(cell_ptr != nullptr);

        REQUIRE(hg.get_neighbor(cell_ptr, Direction::EAST) == hg.get_east(cell_ptr));
        REQUIRE(hg.get_neighbor(cell_ptr, Direction::WEST) == hg.get_west(cell_ptr));
        REQUIRE(hg.get_neighbor(cell_ptr, Direction::NORTHEAST) == hg.get_northeast(cell_ptr));
        REQUIRE(hg.get_neighbor(cell_ptr, Direction::NORTHWEST) == hg.get_northwest(cell_ptr));
        REQUIRE(hg.get_neighbor(cell_ptr, Direction::SOUTHEAST) == hg.get_southeast(cell_ptr));
        REQUIRE(hg.get_neighbor(cell_ptr, Direction::SOUTHWEST) == hg.get_southwest(cell_ptr));
    }
}

TEST_CASE("Hexagonal grid boundary conditions", "[hexagonal_grid_bounds]")
{
    hexagonal_grid hg{HEX_ROWS, HEX_COLS, HEX_LEVELS};

    SECTION("Left edge cells have no WEST neighbor")
    {
        // col=0 cells: indices 0, 5, 10, 15, 20
        for (int row = 0; row < HEX_ROWS; ++row)
        {
            auto edge_cell = hg.search(row * HEX_COLS);
            REQUIRE(edge_cell != nullptr);
            REQUIRE(hg.get_west(edge_cell) == nullptr);
        }
    }

    SECTION("Right edge cells have no EAST neighbor")
    {
        // col=4 cells: indices 4, 9, 14, 19, 24
        for (int row = 0; row < HEX_ROWS; ++row)
        {
            auto edge_cell = hg.search(row * HEX_COLS + HEX_COLS - 1);
            REQUIRE(edge_cell != nullptr);
            REQUIRE(hg.get_east(edge_cell) == nullptr);
        }
    }
}

TEST_CASE("Hexagonal grid maze generation with DFS", "[hexagonal_grid_dfs]")
{
    hexagonal_grid hg{HEX_ROWS, HEX_COLS, HEX_LEVELS};
    randomizer rng{};
    rng.seed(42u);
    dfs algo{};

    SECTION("DFS runs successfully on hexagonal grid")
    {
        bool result = algo.run(&hg, rng);
        REQUIRE(result);
    }

    SECTION("DFS connects cells in hexagonal grid")
    {
        algo.run(&hg, rng);

        // At least one cell should have links (maze is generated)
        bool any_links = false;
        for (int i = 0; i < hg.num_cells(); ++i)
        {
            auto c = hg.search(i);
            if (c && !c->get_links().empty())
            {
                any_links = true;
                break;
            }
        }
        REQUIRE(any_links);
    }
}

TEST_CASE("CLI --hexagonal argument parsing", "[hexagonal_cli]")
{
    args args_handler{};

    SECTION("--hexagonal flag is recognized")
    {
        vector<string> args_vec = {args::HEXAGONAL_OPTION_STR};
        bool parsed = args_handler.parse(args_vec);
        REQUIRE(parsed);

        auto result = args_handler.get();
        REQUIRE(result.has_value());

        auto map = result.value();
        REQUIRE(map.count(args::HEXAGONAL_WORD_STR) > 0);
        REQUIRE(map.at(args::HEXAGONAL_WORD_STR) == args::TRUE_VALUE);
    }

    SECTION("--hexagonal combined with other arguments")
    {
        vector<string> args_vec = {
            args::ROW_FLAG_STR, "5",
            args::COLUMN_FLAG_STR, "5",
            args::HEXAGONAL_OPTION_STR
        };
        bool parsed = args_handler.parse(args_vec);
        REQUIRE(parsed);

        auto result = args_handler.get();
        REQUIRE(result.has_value());

        auto map = result.value();
        REQUIRE(map.count(args::HEXAGONAL_WORD_STR) > 0);
        REQUIRE(map.at(args::HEXAGONAL_WORD_STR) == args::TRUE_VALUE);
        REQUIRE(map.at(args::ROW_WORD_STR) == "5");
        REQUIRE(map.at(args::COLUMN_WORD_STR) == "5");
    }

    SECTION("Without --hexagonal flag, key is absent")
    {
        vector<string> args_vec = {
            args::ROW_FLAG_STR, "5",
            args::COLUMN_FLAG_STR, "5"
        };
        bool parsed = args_handler.parse(args_vec);
        REQUIRE(parsed);

        auto result = args_handler.get();
        REQUIRE(result.has_value());

        auto map = result.value();
        REQUIRE(map.count(args::HEXAGONAL_WORD_STR) == 0);
    }

    SECTION("HEXAGONAL_OPTION_STR constant is correct")
    {
        REQUIRE(string{args::HEXAGONAL_OPTION_STR} == "--hexagonal");
    }

    SECTION("HEXAGONAL_WORD_STR constant is correct")
    {
        REQUIRE(string{args::HEXAGONAL_WORD_STR} == "hexagonal");
    }
}
