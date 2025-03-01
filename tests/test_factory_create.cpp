#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <MazeBuilder/maze_builder.h>

using namespace mazes;
using namespace std;


TEST_CASE( "Test factory create1", "[create1]" ) {
    auto m = factory::create(configurator().rows(10).columns(10).levels(10)._algo(algo::BINARY_TREE).seed(12345));
    REQUIRE(m.has_value());
}