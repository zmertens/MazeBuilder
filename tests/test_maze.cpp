#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <memory>
#include <random>

#include <MazeBuilder/maze_builder.h>

using namespace mazes;
using namespace std;

using maze_ptr = unique_ptr<maze>;

TEST_CASE( "Test maze init", "[maze init]" ) {



    BENCHMARK("Benchmark 10x10 mazes") {
        static constexpr auto NUM_ROWS = 10, NUM_COLS = 10, HEIGHT = 10;
        static constexpr auto OFFSET_X = 10, OFFSET_Z = 10;
        // mazes::builder builder;
        // auto maze1 = builder.rows(NUM_ROWS).columns(NUM_COLS).height(HEIGHT)
            // .offset_x(OFFSET_X).offset_z(OFFSET_Z)
            // .build();
        progress p{};
        // computations::compute_geometry(cref(maze1));
        auto elapsed = p.elapsed_s();
        // REQUIRE(maze1->columns == NUM_COLS);
        // REQUIRE(maze1->rows == NUM_ROWS);
        // REQUIRE(maze1->height == HEIGHT);
        // REQUIRE(maze1->offset_x == OFFSET_X);
        // REQUIRE(maze1->offset_z == OFFSET_Z);
        REQUIRE(elapsed <= 1000.0);
    };
}
