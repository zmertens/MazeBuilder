#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/topology.h>
#include <MazeBuilder/runtime_app.h>

#include <string>
#include <string_view>

using namespace mazes;

TEST_CASE("topology::parse handles an empty string", "[topology][parse]")
{
    const auto topo = topology::parse("");

    REQUIRE(topo.rows == 0u);
    REQUIRE(topo.columns == 0u);
    REQUIRE(topo.cells.empty());
}

TEST_CASE("topology::parse reads a fully-walled 1x2 grid", "[topology][parse]")
{
    constexpr std::string_view grid =
        "+---+---+\n"
        "|   |   |\n"
        "+---+---+";

    const auto topo = topology::parse(grid);

    REQUIRE(topo.rows == 1u);
    REQUIRE(topo.columns == 2u);

    const auto *left = topo.at(0u, 0u);
    const auto *right = topo.at(0u, 1u);
    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);

    REQUIRE(left->north());
    REQUIRE(left->south());
    REQUIRE(left->west());
    REQUIRE(left->east());

    REQUIRE(right->north());
    REQUIRE(right->south());
    REQUIRE(right->west());
    REQUIRE(right->east());
}

TEST_CASE("topology::parse detects an open passage between cells", "[topology][parse]")
{
    constexpr std::string_view grid =
        "+---+---+\n"
        "|       |\n"
        "+---+---+";

    const auto topo = topology::parse(grid);

    REQUIRE(topo.rows == 1u);
    REQUIRE(topo.columns == 2u);

    const auto *left = topo.at(0u, 0u);
    const auto *right = topo.at(0u, 1u);
    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);

    // No wall character between the cells means the passage is open (linked).
    REQUIRE_FALSE(left->east());
    REQUIRE_FALSE(right->west());

    // Outer boundary walls remain intact.
    REQUIRE(left->north());
    REQUIRE(left->south());
    REQUIRE(left->west());
    REQUIRE(right->north());
    REQUIRE(right->south());
    REQUIRE(right->east());
}

TEST_CASE("topology::at returns nullptr out of bounds", "[topology][bounds]")
{
    constexpr std::string_view grid =
        "+---+---+\n"
        "|   |   |\n"
        "+---+---+";

    const auto topo = topology::parse(grid);

    REQUIRE(topo.at(1u, 0u) == nullptr);
    REQUIRE(topo.at(0u, 2u) == nullptr);
}

TEST_CASE("topology::parse round-trips a runtime_app generated maze", "[topology][parse][apply]")
{
    auto app = mazes::runtime_app::instance();
    REQUIRE(app);

    constexpr std::string_view request = "--rows=6 --columns=6 --levels=1 --algo=dfs --seed=42 --output=stdout";
    const std::string_view generated = app->apply(request);
    REQUIRE_FALSE(generated.empty());

    const auto topo = topology::parse(generated);

    REQUIRE(topo.rows == 6u);
    REQUIRE(topo.columns == 6u);
    REQUIRE(topo.cells.size() == 36u);

    // A well-formed maze has at least one open passage somewhere.
    bool found_open_passage = false;
    for (unsigned int row = 0u; row < topo.rows && !found_open_passage; ++row)
    {
        for (unsigned int col = 0u; col < topo.columns && !found_open_passage; ++col)
        {
            const auto *walls = topo.at(row, col);
            REQUIRE(walls != nullptr);
            if (!walls->north() || !walls->south() || !walls->east() || !walls->west())
            {
                found_open_passage = true;
            }
        }
    }
    REQUIRE(found_open_passage);
}
