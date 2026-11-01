#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <functional>
#include <ranges>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/mask.h>
#include <MazeBuilder/masked_grid.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/string_utils.h>

#include <fmt/format.h>

using namespace mazes;
using namespace std;

#if defined(MAZE_BENCHMARK)

TEST_CASE("E2E benchmarking with lab::dfs", "[lab::dfs]")
{
    randomizer rng{};
    const mask m1{mask::from_source(string_utils::make_mask_source(std::string(200, 'X') + std::string(300, 'o') + std::string(200, 'x') + std::string(300, '.')))};
    masked_grid g1{m1};
    const auto cols = static_cast<int>(m1.columns());
    const auto open = [&m1, cols](const int index)
    {
        return m1(static_cast<unsigned int>(index / cols % static_cast<int>(m1.rows())), static_cast<unsigned int>(index % cols));
    };
    REQUIRE(lab::dfs(g1.operations(), rng, open));

    BENCHMARK("DFS on masked grid")
    {
        return lab::dfs(g1.operations(), rng, open);
    };
}
#endif // MAZE_BENCHMARK

TEST_CASE("Lab can link/unlink cells", "[links]")
{

    SECTION("Bidirectional linking works correctly")
    {
        auto cell1 = std::make_shared<cell>(cell::cell_area{1, 1});
        auto cell2 = std::make_shared<cell>(cell::cell_area{2, 2});

        // Initially cells should not be linked
        REQUIRE_FALSE(cell1->is_linked(cell2));
        REQUIRE_FALSE(cell2->is_linked(cell1));

        // Link cells bidirectionally (default behavior)
        lab::link(cell1, cell2);

        // Both cells should now be linked to each other
        REQUIRE(cell1->is_linked(cell2));
        REQUIRE(cell2->is_linked(cell1));
    }

    SECTION("Unidirectional linking works correctly")
    {
        auto cell1 = std::make_shared<cell>(cell::cell_area{3, 3});
        auto cell2 = std::make_shared<cell>(cell::cell_area{4, 4});

        // Link unidirectionally
        lab::link(cell1, cell2, false);

        // Only cell1 should be linked to cell2
        REQUIRE(cell1->is_linked(cell2));
        REQUIRE_FALSE(cell2->is_linked(cell1));
    }

    SECTION("Unlinking works correctly")
    {
        auto cell1 = std::make_shared<cell>(cell::cell_area{5, 5});
        auto cell2 = std::make_shared<cell>(cell::cell_area{6, 6});

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

    SECTION("Unidirectional unlinking works correctly")
    {
        auto cell1 = std::make_shared<cell>(cell::cell_area{7, 7});
        auto cell2 = std::make_shared<cell>(cell::cell_area{8, 8});

        // Link bidirectionally first
        lab::link(cell1, cell2);

        // Unlink unidirectionally
        lab::unlink(cell1, cell2, false);

        // Only cell2 should still be linked to cell1
        REQUIRE_FALSE(cell1->is_linked(cell2));
        REQUIRE(cell2->is_linked(cell1));
    }

    SECTION("Null pointer handling in link")
    {
        auto cell1 = std::make_shared<cell>(cell::cell_area{9, 9});

        // These should not crash
        lab::link(nullptr, cell1);
        lab::link(cell1, nullptr);
        lab::link(nullptr, nullptr);

        // Cell should still be in valid state
        REQUIRE(cell1->get_area() == cell::cell_area{9, 9});
    }

    SECTION("Null pointer handling in unlink")
    {
        auto cell1 = std::make_shared<cell>(cell::cell_area{10, 10});

        // These should not crash
        lab::unlink(nullptr, cell1);
        lab::unlink(cell1, nullptr);
        lab::unlink(nullptr, nullptr);

        // Cell should still be in valid state
        REQUIRE(cell1->get_area() == cell::cell_area{10, 10});
    }

    SECTION("Multiple link/unlink operations")
    {
        auto cell1 = std::make_shared<cell>(cell::cell_area{11, 11});
        auto cell2 = std::make_shared<cell>(cell::cell_area{12, 12});
        auto cell3 = std::make_shared<cell>(cell::cell_area{13, 13});

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

    SECTION("Link same cell multiple times")
    {
        auto cell1 = std::make_shared<cell>(cell::cell_area{14, 14});
        auto cell2 = std::make_shared<cell>(cell::cell_area{15, 15});

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
