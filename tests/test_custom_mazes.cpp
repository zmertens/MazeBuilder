#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <memory>
#include <random>
#include <type_traits>

#include <MazeBuilder/maze_builder.h>

using namespace mazes;
using namespace std;

TEST_CASE( "Test maze init", "[maze init]" ) {

     BENCHMARK("Benchmark stringify") {
        auto maze_opt = factory::create({100, 100, 100});

        REQUIRE(maze_opt.has_value());

        const auto& g = maze_opt.value()->get_grid();

        auto s = computations::stringify(cref(maze_opt.value()));

        REQUIRE(!s.empty());
     };

     STATIC_CHECK(std::is_nothrow_move_constructible_v<grid>);
}
