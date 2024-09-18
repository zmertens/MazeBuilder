#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <memory>
#include <random>

#include "maze_thread_safe.h"

using namespace mazes;
using namespace std;

TEST_CASE( "Test maze init", "[maze init]" ) {
    mt19937 rng { 0 };
    auto get_int = [&rng](auto low, auto high)->auto {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng);
    };

    shared_ptr<maze_thread_safe> maze1 = make_shared<maze_thread_safe>(
        maze_types::DFS, cref(get_int), cref(rng),
        10, 10, 10, 1);


    REQUIRE(maze1->get_length() == 10);
    REQUIRE(maze1->get_width() == 10);
}
