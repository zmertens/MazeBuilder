#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <memory>
#include <random>
#include <type_traits>

#include <MazeBuilder/maze_builder.h>

using namespace mazes;
using namespace std;

TEST_CASE( "Test stringify ", "[stringify]" ) {

     BENCHMARK("Benchmark stringify") {
        auto maze_opt = factory::create(configurator().rows(10).columns(10).levels(10)._algo(algo::BINARY_TREE).seed(12345));

        REQUIRE(maze_opt.has_value());

        const auto& g = maze_opt.value()->get_grid();

        auto s = stringz::stringify(cref(maze_opt.value()));

        REQUIRE(!s.empty());
     };
}

//TEST_CASE("Test objectify ", "[objectify]") {
//
//    auto maze_opt = factory::create({ 100, 100, 100 });
//
//    REQUIRE(maze_opt.has_value());
//
//    const auto& g = maze_opt.value()->get_grid();
//
//    vector<tuple<int, int, int, int>> vertices;
//
//    vector<vector<uint32_t>> faces;
//
//    stringz::objectify(cref(maze_opt.value()), vertices, faces);
//
//    REQUIRE(!vertices.empty());
//
//    REQUIRE(!faces.empty());
//}
