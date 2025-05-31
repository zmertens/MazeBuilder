//#include <catch2/catch_test_macros.hpp>
//
//#include <MazeBuilder/maze_builder.h>
//
//#include <functional>
//#include <tuple>
//#include <memory>
//
//using namespace mazes;
//using namespace std;
//
//TEST_CASE("Distances initialization and basic operations", "[distances]") {
//    // Initialize with root index 0
//    distances dist(0);
//
//    SECTION("Root index has distance 0") {
//        REQUIRE(dist[0] == 0);
//    }
//
//    SECTION("Set and retrieve distances") {
//        dist.set(1, 5);
//        REQUIRE(dist[1] == 5);
//    }
//
//    SECTION("Check containment of indices") {
//        dist.set(2, 10);
//        REQUIRE(dist.contains(2));
//        REQUIRE_FALSE(dist.contains(3));
//    }
//}
//
//TEST_CASE("path_to finds the shortest path", "[distances]") {
//    auto g = std::make_shared<mazes::grid>(3, 3, 1);
//    g->start_configuration({ 0, 1, 2, 3, 4, 5, 6, 7, 8 });
//    auto cell0 = g->search(0);
//    auto cell1 = g->search(1);
//    auto cell2 = g->search(g->num_cells() - 1);
//
//    cell0->link(cell2, true);
//
//    REQUIRE(cell0);
//    REQUIRE(cell1);
//    REQUIRE(cell2);
//
//    mazes::distances dist(cell0->get_index());
//    auto path = dist.path_to(cell2->get_index(), *g);
//
//    REQUIRE(path);
//    REQUIRE(path->contains(cell0->get_index()));
//    REQUIRE(path->contains(cell2->get_index()));
//    REQUIRE(path->operator[](cell0->get_index()) == 0);
//    REQUIRE(path->operator[](cell2->get_index()) != 0);
//}
//
//
//TEST_CASE("Distances maximum distance calculation", "[distances]") {
//    distances dist(0);
//    dist.set(1, 5);
//    dist.set(2, 10);
//    dist.set(3, 7);
//
//    auto [max_index, max_distance] = dist.max();
//
//    REQUIRE(max_index == 2);
//    REQUIRE(max_distance == 10);
//}
//
//TEST_CASE("Distances collect keys", "[distances]") {
//    distances dist(0);
//    dist.set(1, 5);
//    dist.set(2, 10);
//
//    std::vector<int32_t> keys;
//    dist.collect_keys(keys);
//
//    REQUIRE(keys.size() == 3);
//    REQUIRE(std::find(keys.begin(), keys.end(), 0) != keys.end());
//    REQUIRE(std::find(keys.begin(), keys.end(), 1) != keys.end());
//    REQUIRE(std::find(keys.begin(), keys.end(), 2) != keys.end());
//}
//
//TEST_CASE("Distances path_to method edge cases", "[distances]") {
//    auto g = std::make_shared<mazes::grid>(3, 3, 1);
//    g->start_configuration({ 0, 1, 2, 3, 4, 5, 6, 7, 8 });
//
//    auto cell0 = g->search(0);
//    auto cell1 = g->search(1);
//
//    // Link only cell0 and cell1
//    cell0->link(cell1, true);
//
//    mazes::distances dist(0); // Initialize distances with root index 0
//    dist.set(1, 1);
//
//    SECTION("Path to valid goal index") {
//        auto path = dist.path_to(1, *g);
//        REQUIRE(path->contains(0) == true);
//        REQUIRE(path->contains(1) == true);
//    }
//
//    SECTION("Path to disconnected cell") {
//        auto path = dist.path_to(2, *g);
//        REQUIRE(path->contains(2) == false);
//    }
//
//    SECTION("Path to invalid index") {
//        auto path = dist.path_to(100, *g);
//        REQUIRE(path->contains(100) == false);
//    }
//}
