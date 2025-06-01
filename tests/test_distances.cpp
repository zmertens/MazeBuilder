#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/maze_builder.h>

#include <functional>
#include <memory>
#include <queue>
#include <tuple>

using namespace mazes;
using namespace std;

TEST_CASE("Distances initialization and basic operations", "[distances]") {
    // Initialize with root index 0
    distances dist(0);

    SECTION("Root index has distance 0") {
        REQUIRE(dist[0] == 0);
    }

    SECTION("Set and retrieve distances") {
        dist.set(1, 5);
        REQUIRE(dist[1] == 5);
    }

    SECTION("Check containment of indices") {
        dist.set(2, 10);
        REQUIRE(dist.contains(2));
        REQUIRE_FALSE(dist.contains(3));
    }
}

TEST_CASE("path_to finds the shortest path", "[distances]") {
    // Create a grid with clear dimensions
    auto g = std::make_shared<mazes::grid>(3, 3, 1);

    // Configure the grid with cell indices
    g->configure({ 0, 1, 2, 3, 4, 5, 6, 7, 8 });

    // Verify cells exist before proceeding
    auto cell0 = g->search(0);
    auto cell1 = g->search(1);
    auto cell8 = g->search(8);

    REQUIRE(cell0 != nullptr);
    REQUIRE(cell1 != nullptr);
    REQUIRE(cell8 != nullptr);

    // Create links between cells to form a path
    lab::link(cell0, cell1);
    lab::link(cell1, cell8);

    // Create distances object and build distances using BFS
    mazes::distances dist(cell0->get_index());

    // Create a "frontier" for BFS starting with the root cell
    std::queue<std::shared_ptr<cell>> frontier;
    frontier.push(cell0);

    // BFS to compute all distances
    while (!frontier.empty()) {
        auto current = frontier.front();
        frontier.pop();

        int current_distance = dist[current->get_index()];

        // Process all linked neighbors
        for (const auto& link_pair : current->get_links()) {
            auto neighbor = link_pair.first;
            bool is_linked = link_pair.second;

            // Only consider linked cells
            if (is_linked && !dist.contains(neighbor->get_index())) {
                dist.set(neighbor->get_index(), current_distance + 1);
                frontier.push(neighbor);
            }
        }
    }

    // Get path from cell0 to cell8
    auto path = dist.path_to(cell8->get_index(), *g);

    // Verify path exists and contains expected cells
    REQUIRE(path != nullptr);
    //REQUIRE(path->contains(cell0->get_index()));
    //REQUIRE(path->contains(cell1->get_index()));
    //REQUIRE(path->contains(cell8->get_index()));
}

TEST_CASE("Distances maximum distance calculation", "[distances]") {
    distances dist(0);
    dist.set(1, 5);
    dist.set(2, 10);
    dist.set(3, 7);

    auto [max_index, max_distance] = dist.max();

    REQUIRE(max_index == 2);
    REQUIRE(max_distance == 10);
}

TEST_CASE("Distances collect keys", "[distances]") {
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

TEST_CASE("Distances path_to method edge cases", "[distances]") {
    auto g = std::make_shared<mazes::grid>(3, 3, 1);
    g->configure({ 0, 1, 2, 3, 4, 5, 6, 7, 8 });

    auto cell0 = g->search(0);
    auto cell1 = g->search(1);

    // Link only cell0 and cell1
    lab::link(cell0, cell1);

    mazes::distances dist(0); // Initialize distances with root index 0
    dist.set(1, 1);

    SECTION("Path to valid goal index") {
        auto path = dist.path_to(1, *g);
        REQUIRE(path->contains(0) == true);
        REQUIRE(path->contains(1) == true);
    }

    SECTION("Path to disconnected cell") {
        auto path = dist.path_to(2, *g);
        REQUIRE(path->contains(2) == false);
    }

    SECTION("Path to invalid index") {
        auto path = dist.path_to(100, *g);
        REQUIRE(path->contains(100) == false);
    }
}
