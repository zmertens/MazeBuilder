#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/maze_builder.h>

#include <functional>
#include <memory>
#include <queue>
#include <tuple>

using namespace mazes;
using namespace std;

TEST_CASE("Distances initialization and basic operations", "[distances]")
{
    // Initialize with root index 0
    distances dist(0);

    SECTION("Root index has distance 0")
    {
        REQUIRE(dist[0] == 0);
    }

    SECTION("Set and retrieve distances")
    {
        dist.set(1, 5);
        REQUIRE(dist[1] == 5);
    }

    SECTION("Check containment of indices")
    {
        dist.set(2, 10);
        REQUIRE(dist.contains(2));
        REQUIRE_FALSE(dist.contains(3));
    }
}

TEST_CASE("Finds the shortest path", "[shortest paths]")
{
    // Create a grid with clear dimensions
    unique_ptr<grid_interface> g = std::make_unique<mazes::grid>(3, 3, 1);
}

TEST_CASE("Distances maximum distance calculation", "[distances]")
{
    distances dist(0);
    dist.set(1, 5);
    dist.set(2, 10);
    dist.set(3, 7);

    auto [max_index, max_distance] = dist.max();

    REQUIRE(max_index == 2);
    REQUIRE(max_distance == 10);
}

TEST_CASE("Distances collect keys", "[distances]")
{
    distances dist(0);
    dist.set(1, 5);
    dist.set(2, 10);

    std::vector<int32_t> keys;
    dist.collect_keys(keys);

    REQUIRE(keys.size() == 3);
    REQUIRE(std::find(keys.begin(), keys.end(), 0) != keys.end());
    REQUIRE(std::find(keys.begin(), keys.end(), 1) != keys.end());
    REQUIRE(std::find(keys.begin(), keys.end(), 2) != keys.end());
}
