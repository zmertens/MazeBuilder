#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <memory>
#include <random>
#include <type_traits>

#include <MazeBuilder/maze_builder.h>

using namespace mazes;
using namespace std;

TEST_CASE( "Benchmark tools ", "[benchmark tools]" ) {

    static constexpr auto ROWS = 50, COLUMNS = 50, LEVELS = 50;
    static constexpr auto SEED = 12345;
    static constexpr auto ALGO = algo::BINARY_TREE;

    BENCHMARK("Benchmark factory::create") {
        auto maze_opt = factory::create(configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS)._algo(ALGO).seed(SEED));

        REQUIRE(maze_opt.has_value());
    };

    BENCHMARK("Benchmark stringify") {
        auto maze_opt = factory::create(configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS)._algo(ALGO).seed(SEED));

        REQUIRE(maze_opt.has_value());

        auto s = stringz::stringify(cref(maze_opt.value()));

        REQUIRE(!s.empty());
    };

    //BENCHMARK("Benchmark objectify") {

    //    auto maze_opt = factory::create(configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS)._algo(ALGO).seed(SEED));

    //    REQUIRE(maze_opt.has_value());

    //    vector<tuple<int, int, int, int>> vertices;

    //    vector<vector<uint32_t>> faces;

    //    stringz::objectify(cref(maze_opt.value()), vertices, faces);

    //    REQUIRE(!vertices.empty());

    //    REQUIRE(!faces.empty());
    //};
}

