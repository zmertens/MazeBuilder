#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/progress.h>

#include "test_output_dir.h"

#include <fmt/format.h>

#if defined(MAZE_BENCHMARK)

TEST_CASE("E2E benchmarking with runtime", "[benchmark]")
{
    // No whitespace inside the JSON literal: apply() tokenizes on whitespace, which would split this into garbage tokens.
    constexpr std::string_view input = "--json=`["
                                       "{\"rows\":\"100\",\"columns\":\"101\",\"levels\":\"8\",\"seed\":\"3\",\"algo\":\"dfs\"},"
                                       "{\"rows\":\"100\",\"columns\":\"101\",\"levels\":\"4\",\"seed\":\"5\",\"algo\":\"binary_tree\"}"
                                       "]`";

    auto printer = [input](auto &&time, const auto iterations)
    {
        const double total_ms = mazes::progress<>::to_double_from_duration(time);
        fmt::println("Benchmark: input={}", input);
        fmt::print("Benchmark: runtime apply took {:.4f} milliseconds total for {} iterations ({:.6f} ms/apply)\n",
                   total_ms,
                   iterations,
                   total_ms / static_cast<double>(iterations));
    };

    constexpr auto ITERATIONS = 100;

    auto benchmark = [=](auto sv, auto iterations) -> void
    {
        std::ranges::for_each(std::views::iota(0, iterations), [sv](auto)
                              { REQUIRE_FALSE(mazes::runtime_app::instance()->apply(sv).empty()); });
    };

    const auto time = mazes::progress<>::duration(benchmark, input, ITERATIONS);
    printer(time, ITERATIONS);
}
#endif // MAZE_BENCHMARK

TEST_CASE("E2E testing starting with apply", "[apply][obj,json,png]")
{
    std::shared_ptr<mazes::runtime_app> inst = mazes::runtime_app::instance();

    test_output_dir output_dir{"test_runtime_json_array"};
    test_output_dir output_dir2{"test_runtime_obj"};
    test_output_dir output_dir3{"test_runtime_png"};

    const std::filesystem::path output_path = output_dir.path / "test_runtime_output.obj";
    const std::filesystem::path output_path2 = output_dir3.path / "test_runtime_output.png";

    const auto first_output_path = (output_dir.path / "first.txt").generic_string();
    const auto second_output_path = (output_dir.path / "second.txt").generic_string();

    constexpr std::string_view input = "-r100 -c100 --levels=1 -s3 -adfs";
    const std::string input2 = "--json=`[{\"rows\":\"2\",\"columns\":\"3\",\"algo\":\"dfs\",\"output\":\"" +
                               first_output_path + "\"},{\"rows\":\"4\",\"columns\":\"5\",\"algo\":\"sidewinder\",\"output\":\"" +
                               second_output_path + "\"}]`";
    const std::string input3 = "-r10 -c12 --levels=1 -s42 -adfs -d -o " + output_path.string();

    std::string_view result = inst->apply(input);
    REQUIRE_FALSE(result.empty());

    const auto result2A = inst->apply(input2);

    REQUIRE_FALSE(result2A.empty());

    const std::string result2B{inst->apply(input2)};

    REQUIRE_FALSE(result2B.empty());

    const std::string_view result3 = inst->apply(input3);

    REQUIRE_FALSE(result3.empty());
}
