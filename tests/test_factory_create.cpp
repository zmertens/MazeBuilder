#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <MazeBuilder/maze_builder.h>

#include <chrono>
#include <vector>

using namespace mazes;
using namespace std;

static constexpr auto ROWS = 50, COLUMNS = 50, LEVELS = 10;

static constexpr auto ALGO_TO_RUN = algo::DFS;
static constexpr auto SEED = 12345;

TEST_CASE( "Test factory create1", "[create1]" ) {

    static constexpr auto ITERATIONS{ 10 };

    vector<double> durations;
    durations.reserve(ITERATIONS);

    for (auto i{ 0 }; i < ITERATIONS; ++i) {
        durations.emplace_back(chrono::duration_cast<chrono::milliseconds>(
            mazes::progress<chrono::milliseconds, chrono::steady_clock>::duration(
                mazes::factory::create,
                mazes::configurator().columns(COLUMNS).rows(ROWS).levels(LEVELS)
                .distances(false).seed(SEED)._algo(ALGO_TO_RUN))).count());
    }
    
    auto max{ 0 };
    for (const auto& duration : durations) {
        max = (max < duration) ? duration : max;
    }
    REQUIRE(max > 0);

    BENCHMARK("Benchmark factory::create") {
        auto maze_opt = factory::create(configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS)._algo(ALGO_TO_RUN).seed(SEED));

        REQUIRE(maze_opt.has_value());
    };

}
