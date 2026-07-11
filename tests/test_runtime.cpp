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

static mazes::args arguments{};
static mazes::randomizer rng{};

auto inst = mazes::runtime_app::instance();

TEST_CASE("E2E testing starting with apply", "[apply][slow]")
{
    constexpr std::string_view input = "-r100 -c100 --levels=1 -s3 -adfs";

    std::string_view result = inst->apply(input);

    REQUIRE_FALSE(result.empty());
}

TEST_CASE("OBJ output writes file through apply", "[apply][obj]")
{
    const std::filesystem::path output_path = "test_runtime_output.obj";
    std::remove(output_path.string().c_str());

    const std::string input = "-r10 -c12 --levels=1 -s42 -adfs -d -o " + output_path.string();

    const std::string_view result = inst->apply(input);

    REQUIRE(result == "Wrote maze to test_runtime_output.obj");
    REQUIRE(std::filesystem::exists(output_path));

    std::ifstream file{output_path};
    REQUIRE(file.is_open());

    const std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    REQUIRE_FALSE(contents.empty());
    REQUIRE(contents.find("# MazeBuilder Wavefront OBJ") != std::string::npos);
    REQUIRE(contents.find("\nv ") != std::string::npos);
    REQUIRE(contents.find("\nf ") != std::string::npos);

    file.close();
    std::remove(output_path.string().c_str());
}

TEST_CASE("PNG output writes file through apply", "[apply][png]")
{
    const std::filesystem::path output_path = "test_runtime_output.png";
    std::remove(output_path.string().c_str());

    const std::string input = "-r10 -c12 --levels=1 -s42 -adfs -d -o " + output_path.string();

    const std::string_view result = inst->apply(input);

    REQUIRE(result == "Wrote maze to test_runtime_output.png");
    REQUIRE(std::filesystem::exists(output_path));

    std::ifstream file{output_path, std::ios::binary};
    REQUIRE(file.is_open());

    std::array<unsigned char, 8> signature{};
    file.read(reinterpret_cast<char *>(signature.data()), static_cast<std::streamsize>(signature.size()));
    REQUIRE(file.gcount() == static_cast<std::streamsize>(signature.size()));

    const std::array<unsigned char, 8> expected_signature{0x89u, 0x50u, 0x4Eu, 0x47u, 0x0Du, 0x0Au, 0x1Au, 0x0Au};
    REQUIRE(signature == expected_signature);

    file.close();
    std::remove(output_path.string().c_str());
}

#if defined(MAZE_BENCHMARK)
TEST_CASE("Apply lots of applies", "[lots of applies]")
{
    constexpr std::array<std::string_view, 5> input = {"-r100 -c100 --levels=1 -s3 -adfs -d",
                                                       "-r50 -c10 --levels=2 -s6 -adfs",
                                                       "-r10 -c50 --levels=3 -s9 -adfs -d[0:10]",
                                                       "-r2 -c5 --levels=4 -s12 -asidewinder",
                                                       "-r5 -c2 --levels=5 -s15 -abinary_tree"};

    constexpr int rounds = 5;
    for (int i = 0; i < rounds; ++i)
    {
        std::ranges::for_each(input, [](std::string_view in)
        {
            const std::string_view result = inst->apply(in);
            REQUIRE_FALSE(result.empty());
        });
    }
}

#endif // MAZE_BENCHMARK
