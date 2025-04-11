#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <MazeBuilder/maze_builder.h>

#include <chrono>
#include <algorithm>
#include <vector>

using namespace mazes;
using namespace std;

static constexpr auto ROWS = 50, COLUMNS = 50, LEVELS = 10;
static constexpr auto ALGO_TO_RUN = algo::DFS;
static constexpr auto ALGO_S = "dfs";
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
                .distances(false).seed(SEED)._algo(to_algo_from_string(string(ALGO_S))))).count());
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

TEST_CASE("Invalid args when converting algo string", "[invalid args]") {

    vector<string> algos_to_convert = { "dfz", "BINARY_TREE", "adjacentwinder" };

    for (auto a : algos_to_convert) {
        REQUIRE_THROWS_AS(mazes::to_algo_from_string(cref(a)), std::invalid_argument);
    }
}

TEST_CASE("randomizer::get_num_ints generates correct number of integers", "[randomizer]") {
    randomizer rng;
    rng.seed(SEED);

    SECTION("Generate 5 integers in range [0, 10]") {
        auto result = rng.get_num_ints_incl(0, 10);
        REQUIRE(result.size() == 10);
        for (int num : result) {
            REQUIRE(num >= 0);
            REQUIRE(num <= 10);
        }
    }

    SECTION("Generate all integers in range [0, 10]") {
        auto result = rng.get_num_ints_incl(0, 10);
        REQUIRE(result.size() == 10);
        std::sort(result.begin(), result.end());
        REQUIRE(result.cend() != result.cbegin());
    }

    SECTION("Request more integers than available in range [0, 5]") {
        auto result = rng.get_num_ints_incl(0, 5);
        REQUIRE(result.size() == 5);
        for (int num : result) {
            REQUIRE(num >= 0);
            REQUIRE(num <= 5);
        }
    }

    SECTION("Empty range [5, 0]") {
        auto result = rng.get_num_ints_incl(5, 0);
        REQUIRE(result.empty());
    }

    SECTION("Zero integers requested") {
        auto result = rng.get_num_ints_incl(0, -1);
        REQUIRE(result.empty());
    }
}
