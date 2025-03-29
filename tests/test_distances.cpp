#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/maze_builder.h>

#include <memory>

using namespace mazes;
using namespace std;

TEST_CASE( "Test distance and paths", "[paths]" ) {

    // Create cells
    auto root = make_shared<cell>(0);
    auto cell1 = make_shared<cell>(1);
    auto cell2 = make_shared<cell>(2);
    auto goal = make_shared<cell>(3);
    auto disconnected = make_shared<cell>(4);

    // Link cells to form a path: root -> cell1 -> cell2 -> goal
    root->link(root, cell1);
    cell1->link(cell1, cell2);
    cell2->link(cell2, goal);

    // Initialize distances object
    distances d(root);
    d.set(cell1, 1);
    d.set(cell2, 2);
    d.set(goal, 3);

    SECTION("Null goal cell") {
        auto path = d.path_to(nullptr);
        REQUIRE(path == nullptr);
    }

    SECTION("Disconnected goal cell") {
        auto path = d.path_to(disconnected);
        REQUIRE(path == nullptr);
    }

    SECTION("Single cell maze") {
        distances single_cell_distances(root);
        auto path = single_cell_distances.path_to(root);
        REQUIRE(path != nullptr);
        REQUIRE(path->contains(root));
        REQUIRE((*path)[root] == 0);
    }

    SECTION("Circular path") {
        cell2->link(cell2, root); // Create a circular path
        auto path = d.path_to(goal);
        REQUIRE(path != nullptr);
        REQUIRE(path->contains(root));
        REQUIRE(path->contains(cell1));
        REQUIRE(path->contains(cell2));
        REQUIRE(path->contains(goal));
        REQUIRE((*path)[root] == 0);
        REQUIRE((*path)[cell1] == 1);
        REQUIRE((*path)[cell2] == 2);
        REQUIRE((*path)[goal] == 3);
    }

    SECTION("Multiple paths") {
        auto cell3 = make_shared<cell>(5);
        cell1->link(cell1, cell3);
        cell3->link(cell3, goal);
        d.set(cell3, 2);
        d.set(goal, 3);
        auto path = d.path_to(goal);
        REQUIRE(path != nullptr);
        REQUIRE(path->contains(root));
        REQUIRE(path->contains(cell1));
        REQUIRE(path->contains(goal));
        REQUIRE(path->operator[](root) == 0);
        REQUIRE(path->operator[](cell1) == 1);
        // Shortest path should be through cell3
        REQUIRE(path->operator[](goal) == 3);
    }

    SECTION("Multiple paths") {
        auto cell3 = make_shared<cell>(5);
        cell1->link(cell1, cell3);
        cell3->link(cell3, goal);
        d.set(cell3, 2);
        d.set(goal, 3);
        auto path = d.path_to(goal);
        REQUIRE(path != nullptr);
        REQUIRE(path->contains(root));
        REQUIRE(path->contains(cell1));
        REQUIRE(path->contains(goal));
        REQUIRE(path->operator[](root) == 0);
        REQUIRE(path->operator[](cell1) == 1);
        // Shortest path should be through cell3
        REQUIRE(path->operator[](goal) == 3);
    }
}
