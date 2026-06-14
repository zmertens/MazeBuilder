#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <MazeBuilder/runtime_app.h>

static mazes::args arguments{};
static mazes::randomizer rng{};

auto inst = mazes::runtime_app::instance();

TEST_CASE("E2E testing starting with apply", "[apply]")
{
    constexpr std::string_view input = "-r100 -c100 --levels=1 -s3 -adfs";

    std::string_view result = inst->apply(input);

    REQUIRE_FALSE(result.empty());
}

#if defined(MAZE_BENCHMARK)
TEST_CASE("Apply lots of applies", "[lots of applies]")
{
    constexpr std::array<std::string_view, 5> input = {"-r100 -c100 --levels=1 -s3 -adfs",
                                                       "-r50 -c50 --levels=1 -s3 -adfs",
                                                       "-r10 -c10 --levels=1 -s3 -adfs",
                                                       "-r5 -c5 --levels=1 -s3 -adfs",
                                                       "-r2 -c2 --levels=1 -s3 -adfs"};

    BENCHMARK("E2E application benchmarking and testing")
    {
        std::ranges::for_each(input, [](std::string_view in)
                              {
        std::string_view result = inst->apply(in);

        REQUIRE_FALSE(result.empty()); });
    };
}

#endif // MAZE_BENCHMARK
