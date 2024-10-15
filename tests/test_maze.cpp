#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <memory>
#include <random>

#include <MazeBuilder/maze_builder.h>

using namespace mazes;
using namespace std;

TEST_CASE( "Test maze init", "[maze init]" ) {
    mt19937 rng { 0 };
    auto get_int = [&rng](auto low, auto high)->auto {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng);
    };

    shared_ptr<maze_builder> maze1 = make_shared<maze_builder>(10, 10, 10);


    REQUIRE(maze1->get_length() == 10);
    REQUIRE(maze1->get_width() == 10);
}

TEST_CASE( "Test maze progress", "[maze progress]") {
    mt19937 rng { 0 };
    auto get_int = [&rng](auto low, auto high)->auto {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng);
    };

    shared_ptr<maze_builder> maze1 = make_shared<maze_builder>(10, 10, 10);

    SECTION("Check no compute progress") {
        maze1->start_progress();
        maze1->stop_progress();
        REQUIRE(maze1->get_progress_in_seconds() <= 10.0);
        REQUIRE(maze1->get_progress_in_ms() <= 1.0);
    }

    SECTION("Check compute_str progresss") {
        maze1->start_progress();
        auto s = maze1->to_str(maze_types::DFS, cref(get_int), cref(rng));
        maze1->stop_progress();
        REQUIRE(maze1->get_progress_in_seconds() > 0);
    }
}

TEST_CASE( "Test maze compute geometry", "[maze geometry]") {
    mt19937 rng { 0 };
    auto get_int = [&rng](auto low, auto high)->auto {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng);
    };

    shared_ptr<maze_builder> maze1 = make_shared<maze_builder>(10, 10, 10);
    // Random block types
    auto block_type = -1;
    maze1->compute_geometry(maze_types::DFS, cref(get_int), cref(rng), block_type);
    REQUIRE(!maze1->to_wavefront_obj_str().empty());
    REQUIRE(maze1->get_vertices_size() > 0);
}
