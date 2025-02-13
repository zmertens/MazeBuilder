#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <memory>
#include <random>
#include <iostream>

#include <MazeBuilder/maze_builder.h>

using namespace mazes;
using namespace std;

TEST_CASE( "Test maze init", "[maze init]" ) {

    vector<vector<bool>> m1 {
        {true, false, false},
        {true, true, false},
        {false, true, true}
    };


    // BENCHMARK("Benchmark stringify") {
        auto maze_opt = factory::create(cref(m1));

        REQUIRE(maze_opt.has_value());

        auto s = maze::stringify(maze_opt.value());

        REQUIRE(!s.empty());
    // };
}
