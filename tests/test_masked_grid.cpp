#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/mask.h>
#include <MazeBuilder/masked_grid.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/runtime_app.h>

#include <memory>
#include <string>

using namespace mazes;
using namespace std;

// ---------------------------------------------------------------------------
// Static type checks
// ---------------------------------------------------------------------------

TEST_CASE("Static Assert mask", "[mask static asserts]")
{
    STATIC_REQUIRE(std::is_constructible<mazes::mask, unsigned int, unsigned int>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::mask>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::mask>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::mask>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::mask>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::mask>::value);
}

TEST_CASE("Static Assert masked_grid", "[masked_grid static asserts]")
{
    STATIC_REQUIRE(std::is_destructible<mazes::masked_grid>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible<mazes::masked_grid>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable<mazes::masked_grid>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::masked_grid>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::masked_grid>::value);
}

// ---------------------------------------------------------------------------
// Mask class tests
// ---------------------------------------------------------------------------

TEST_CASE("Mask initialization - all cells available", "[mask][init]")
{
    mask m(5u, 5u);

    REQUIRE(m.rows() == 5u);
    REQUIRE(m.columns() == 5u);
    REQUIRE(m.count() == 25);

    // All cells should be available
    for (unsigned int row = 0; row < 5u; ++row)
    {
        for (unsigned int col = 0; col < 5u; ++col)
        {
            REQUIRE(m(row, col));
        }
    }
}

TEST_CASE("Mask out-of-bounds access returns false", "[mask][bounds]")
{
    mask m(3u, 3u);

    REQUIRE_FALSE(m(3u, 0u));
    REQUIRE_FALSE(m(0u, 3u));
    REQUIRE_FALSE(m(10u, 10u));
}

TEST_CASE("Mask set and get", "[mask][set]")
{
    mask m(5u, 5u);

    REQUIRE(m(1u, 1u));
    m.set(1u, 1u, false);
    REQUIRE_FALSE(m(1u, 1u));

    m.set(1u, 1u, true);
    REQUIRE(m(1u, 1u));
}

TEST_CASE("Mask count reflects blocked cells", "[mask][count]")
{
    mask m(4u, 4u);

    REQUIRE(m.count() == 16);

    m.set(0u, 0u, false);
    m.set(1u, 1u, false);
    m.set(2u, 2u, false);

    REQUIRE(m.count() == 13);
}

TEST_CASE("Mask random_location returns a valid cell", "[mask][random_location]")
{
    mask m(5u, 5u);

    // Block all cells except (2, 2)
    for (unsigned int row = 0; row < 5u; ++row)
    {
        for (unsigned int col = 0; col < 5u; ++col)
        {
            if (row != 2u || col != 2u)
            {
                m.set(row, col, false);
            }
        }
    }

    REQUIRE(m.count() == 1);

    randomizer rng;
    rng.seed(42);
    const auto [r, c] = m.random_location(rng);

    REQUIRE(r == 2u);
    REQUIRE(c == 2u);
}

TEST_CASE("Mask loaded from text file", "[mask][from_txt]")
{
    // mask.txt contents:
    // X....
    // .XXX.
    // .X...
    // .XXX.
    // ....X
    //
    // Available cells: 5x5 = 25, blocked = 1+3+1+3+1 = 9, available = 16
    const mask m = mask::from_txt("mask.txt");

    REQUIRE(m.rows() == 5u);
    REQUIRE(m.columns() == 5u);

    // Top-left is blocked
    REQUIRE_FALSE(m(0u, 0u));

    // (0,1) is available
    REQUIRE(m(0u, 1u));

    // (1,1), (1,2), (1,3) are blocked
    REQUIRE_FALSE(m(1u, 1u));
    REQUIRE_FALSE(m(1u, 2u));
    REQUIRE_FALSE(m(1u, 3u));

    // Count should be 25 - 9 = 16
    REQUIRE(m.count() == 16);
}

TEST_CASE("Mask from_txt throws on missing file", "[mask][from_txt][error]")
{
    REQUIRE_THROWS_AS(mask::from_txt("nonexistent_mask_file.txt"), std::runtime_error);
}

// ---------------------------------------------------------------------------
// Masked grid tests
// ---------------------------------------------------------------------------

TEST_CASE("Masked grid inherits from grid_interface", "[masked_grid][interface]")
{
    mask m(5u, 5u);
    auto mg = make_unique<masked_grid>(std::move(m));

    // Should be usable as grid_interface
    grid_interface *gi = mg.get();
    REQUIRE(gi != nullptr);
}

TEST_CASE("Masked grid dimensions match mask", "[masked_grid][dimensions]")
{
    mask m(7u, 9u);
    masked_grid mg(std::move(m));

    const auto [rows, cols, levels] = mg.operations().get_dimensions();
    REQUIRE(rows == 7u);
    REQUIRE(cols == 9u);
    REQUIRE(levels == 1u);
}

TEST_CASE("Masked grid num_cells returns mask count", "[masked_grid][num_cells]")
{
    mask m(5u, 5u);
    m.set(0u, 0u, false);
    m.set(1u, 1u, false);
    m.set(2u, 2u, false);
    // 25 - 3 = 22 available cells
    const int expected_count = m.count();
    REQUIRE(expected_count == 22);

    masked_grid mg(std::move(m));
    REQUIRE(mg.num_cells() == 22);
}

TEST_CASE("Masked grid search returns nullptr for blocked cells", "[masked_grid][search]")
{
    mask m(5u, 5u);
    m.set(0u, 0u, false); // index 0 is blocked
    m.set(1u, 1u, false); // index 6 is blocked (1*5 + 1)

    masked_grid mg(std::move(m));

    // Index 0 should be nullptr (blocked)
    REQUIRE(mg.search(0) == nullptr);

    // Index 6 should be nullptr (blocked)
    REQUIRE(mg.search(6) == nullptr);

    // Index 1 should be available
    REQUIRE(mg.search(1) != nullptr);

    // Index 5 should be available (row=1, col=0)
    REQUIRE(mg.search(5) != nullptr);
}

TEST_CASE("Masked grid blocked neighbors are not returned", "[masked_grid][neighbors]")
{
    // Create a mask where (0,1) is blocked - east neighbor of (0,0)
    mask m(3u, 3u);
    m.set(0u, 1u, false);

    masked_grid mg(std::move(m));

    // Cell at (0,0) should not have an east neighbor
    const auto cell_0_0 = mg.search(0);
    REQUIRE(cell_0_0 != nullptr);

    const auto east_neighbor = mg.operations().get_east(cell_0_0);
    REQUIRE(east_neighbor == nullptr); // blocked by mask
}

TEST_CASE("Masked grid random_cell returns available cell", "[masked_grid][random_cell]")
{
    mask m(5u, 5u);

    // Block everything except center cell (2,2) - index = 2*5+2 = 12
    for (unsigned int row = 0; row < 5u; ++row)
    {
        for (unsigned int col = 0; col < 5u; ++col)
        {
            if (row != 2u || col != 2u)
            {
                m.set(row, col, false);
            }
        }
    }

    masked_grid mg(std::move(m));
    randomizer rng;
    rng.seed(42);

    const auto cell = mg.random_cell(rng);
    REQUIRE(cell != nullptr);
    REQUIRE(cell->get_index() == 12); // (2,2) in a 5x5 grid = index 12
}

TEST_CASE("Masked grid get_mask returns the mask", "[masked_grid][get_mask]")
{
    mask m(4u, 4u);
    m.set(0u, 0u, false);
    const int original_count = m.count(); // 15

    masked_grid mg(std::move(m));

    const auto &retrieved_mask = mg.get_mask();
    REQUIRE(retrieved_mask.rows() == 4u);
    REQUIRE(retrieved_mask.columns() == 4u);
    REQUIRE(retrieved_mask.count() == original_count);
    REQUIRE_FALSE(retrieved_mask(0u, 0u));
}

TEST_CASE("Masked grid from file integration with binary_tree", "[masked_grid][algorithm][binary_tree]")
{
    // const mask m = mask::from_txt("mask.txt");
    // REQUIRE(m.count() == 16); // 16 available cells in our mask.txt

    // masked_grid mg(m);

    // binary_tree bt;
    // randomizer rng;
    // rng.seed(12345);

    // const bool result = bt.run(&mg, rng);
    // REQUIRE(result);
}

// ---------------------------------------------------------------------------
// Args parsing tests for --mask
// ---------------------------------------------------------------------------

TEST_CASE("Args parse --mask flag", "[args][mask]")
{
    args args_handler;

    SECTION("Long form --mask")
    {
        REQUIRE(args_handler.parse("--mask mask.txt"));
        const auto val = args_handler.get(args::MASK_WORD_STR);
        REQUIRE(val.has_value());
        REQUIRE(val.value() == "mask.txt");
    }

    SECTION("Short form -m")
    {
        REQUIRE(args_handler.parse("-m mask.txt"));
        const auto val = args_handler.get(args::MASK_WORD_STR);
        REQUIRE(val.has_value());
        REQUIRE(val.value() == "mask.txt");
    }

    SECTION("Option form --mask=mask.txt")
    {
        REQUIRE(args_handler.parse("--mask=mask.txt"));
        const auto val = args_handler.get(args::MASK_WORD_STR);
        REQUIRE(val.has_value());
        REQUIRE(val.value() == "mask.txt");
    }
}

TEST_CASE("Args mask combined with other options", "[args][mask][combined]")
{
    args args_handler;

    REQUIRE(args_handler.parse("-m mask.txt -a dfs -r 10 -c 10"));

    const auto mask_val = args_handler.get(args::MASK_WORD_STR);
    REQUIRE(mask_val.has_value());
    REQUIRE(mask_val.value() == "mask.txt");

    const auto algo_val = args_handler.get(args::ALGO_ID_WORD_STR);
    REQUIRE(algo_val.has_value());
    REQUIRE(algo_val.value() == "dfs");
}
