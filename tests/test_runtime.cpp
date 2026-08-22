#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/progress.h>

#include "test_output_dir.h"

#include <fmt/format.h>

namespace {
    std::shared_ptr<mazes::runtime_app> inst = mazes::runtime_app::instance();
}

#if defined(MAZE_BENCHMARK)

TEST_CASE("E2E benchmarking with runtime", "[benchmark]")
{
    // No whitespace inside the JSON literal: apply() tokenizes on whitespace, which would split this into garbage tokens.
    constexpr std::string_view input = "--json=`{\"rows\":\"100\",\"columns\":\"100\",\"levels\":\"10\",\"seed\":\"3\",\"algo\":\"dfs\"}`";

    auto printer = [](auto &&time, const auto iterations)
    {
        const double total_ms = mazes::progress<>::to_double_from_duration(time);
        fmt::print("Benchmark: runtime apply took {:.4f} milliseconds total for {} iterations ({:.6f} ms/apply)\n",
                   total_ms,
                   iterations,
                   total_ms / static_cast<double>(iterations));
    };

    constexpr auto ITERATIONS = 1'000;

    auto benchmark = [=](auto sv, auto iterations) -> void
    {
        std::ranges::for_each(std::views::iota(0, iterations), [sv](auto)
                              { REQUIRE_FALSE(inst->apply(sv).empty()); });
    };
    (void)mazes::progress<>::duration(benchmark, input, 1);
    const auto time = mazes::progress<>::duration(benchmark, input, ITERATIONS);
    printer(time, ITERATIONS);
}

#endif // MAZE_BENCHMARK

TEST_CASE("E2E testing starting with apply", "[apply][slow]")
{
    constexpr std::string_view input = "-r100 -c100 --levels=1 -s3 -adfs";

    std::string_view result = inst->apply(input);

    REQUIRE_FALSE(result.empty());
}

TEST_CASE("OBJ output writes file through apply", "[apply][obj]")
{
    test_output_dir output_dir{"test_runtime_obj"};
    const std::filesystem::path output_path = output_dir.path / "test_runtime_output.obj";

    const std::string input = "-r10 -c12 --levels=1 -s42 -adfs -d -o " + output_path.string();

    const auto result = inst->apply(input);
    
    REQUIRE_FALSE(result.empty());
}

TEST_CASE("JSON array output processes each object", "[apply][json][array]")
{
    test_output_dir output_dir{"test_runtime_json_array"};
    const auto first_output_path = (output_dir.path / "first.txt").generic_string();
    const auto second_output_path = (output_dir.path / "second.txt").generic_string();

    const std::string input = "--json=`[{\"rows\":\"2\",\"columns\":\"3\",\"algo\":\"dfs\",\"output\":\"" +
                              first_output_path + "\"},{\"rows\":\"4\",\"columns\":\"5\",\"algo\":\"sidewinder\",\"output\":\"" +
                              second_output_path + "\"}]`";

    const std::string result{inst->apply(input)};

    REQUIRE_FALSE(result.empty());
}

TEST_CASE("PNG output writes file through apply", "[apply][png]")
{
    test_output_dir output_dir{"test_runtime_png"};
    const std::filesystem::path output_path = output_dir.path / "test_runtime_output.png";

    const std::string input = "-r10 -c12 --levels=1 -s42 -adfs -d -o " + output_path.string();

    const std::string_view result = inst->apply(input);

    REQUIRE_FALSE(result.empty());
}
