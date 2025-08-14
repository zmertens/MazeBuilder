#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/randomizer.h>

#include <algorithm>
#include <chrono>
#include <vector>

using namespace mazes;
using namespace std;

static constexpr auto ROWS = 50, COLUMNS = 50, LEVELS = 10;
static constexpr auto ALGO_TO_RUN = algo::DFS;
static constexpr auto ALGO_S = "dfs";
static constexpr auto SEED = 12345;

TEST_CASE( "Test factory create1", "[create1]" ) {


#if defined(MAZE_BENCHMARK)

    grid_factory factory1{};

    BENCHMARK("Benchmark factory::create") {

        [[maybe_unused]]
        auto g = factory1.create(configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO_TO_RUN).seed(SEED));
    };
#endif
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

    static constexpr auto low = 0, high = 10;

    SECTION("Validate random number values are within specific range") {

        auto result = rng.get_num_ints_incl(low, high);
        REQUIRE(result.size() == high + 1);
        for (int num : result) {
            REQUIRE(num >= low);
            REQUIRE(num <= high);
        }
    }

    SECTION("Generate all integers in a range") {
        auto result = rng.get_num_ints_incl(low, high);
        REQUIRE(result.size() == high + 1);
        std::sort(result.begin(), result.end());
        REQUIRE(result.cend() != result.cbegin());
    }

    SECTION("Empty range [high, low]") {
        auto result = rng.get_num_ints_incl(high, low);
        REQUIRE(result.empty());
    }

    SECTION("Zero integers requested") {
        auto result = rng.get_num_ints_incl(0, -1);
        REQUIRE(result.empty());
    }
}
