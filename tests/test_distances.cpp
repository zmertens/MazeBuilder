#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/maze_builder.h>

#include <memory>

using namespace mazes;
using namespace std;

TEST_CASE( "Test distance and paths", "[paths]" ) {

    // Create cells
    auto root = std::make_shared<cell>(0);
    auto cell1 = std::make_shared<cell>(1);
    auto cell2 = std::make_shared<cell>(2);
    auto goal = std::make_shared<cell>(3);

    // Link cells
    root->link(root, cell1);
    cell1->link(cell1, cell2);
    cell2->link(cell2, goal);

    // Initialize distances object
    distances d(root);
    d.set(cell1, 1);
    d.set(cell2, 2);
    d.set(goal, 3);

    // Get path to goal
    auto path = d.path_to(goal);

    // Verify the path
    REQUIRE_FALSE(path == nullptr);
    REQUIRE(path->contains(root));
    REQUIRE(path->contains(cell1));
    REQUIRE(path->contains(cell2));
    REQUIRE(path->contains(goal));
    REQUIRE((*path)[root] == 0);
    REQUIRE((*path)[cell1] == 1);
    REQUIRE((*path)[cell2] == 2);
    REQUIRE((*path)[goal] == 3);
}
