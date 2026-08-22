#include <catch2/catch_test_macros.hpp>

#include <MazeBuilder/runtime_app.h>

#include "test_output_dir.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

using namespace mazes;
using namespace std;

namespace {
    std::shared_ptr<runtime_app> app = runtime_app::instance();
}

// ---------------------------------------------------------------------------
// Helper functions
// ---------------------------------------------------------------------------

namespace
{
    void create_simple_mask_file(const std::string& filename)
    {
        std::ofstream out(filename);
        out << "...........\n";
        out << "..XXX......\n";
        out << "..X.X......\n";
        out << "..XXX......\n";
        out << "...........\n";
        out.close();
    }

    void create_complex_mask_file(const std::string& filename)
    {
        std::ofstream out(filename);
        out << "XXXXXXXXXXXXXXXX\n";
        out << "X..............X\n";
        out << "X..XXXX..XXXX..X\n";
        out << "X..X.......X...X\n";
        out << "X..X.......X...X\n";
        out << "X..XXXX..XXXX..X\n";
        out << "X..............X\n";
        out << "XXXXXXXXXXXXXXXX\n";
        out.close();
    }
}

// ---------------------------------------------------------------------------
// Masked maze state tests
// ---------------------------------------------------------------------------

TEST_CASE("Masked maze with binary_tree algorithm", "[masked_maze][binary_tree]")
{
    test_output_dir output_dir{ "masked_maze_bt" };
    const std::string mask_file = output_dir.path.string() + "/test_bt_mask.txt";
    create_simple_mask_file(mask_file);

    REQUIRE(std::filesystem::exists(mask_file));

    REQUIRE(app);
    const auto result = app->apply("-m " + mask_file + " -a binary_tree -o stdout");

    REQUIRE_FALSE(result.empty());
    REQUIRE(result.find("Error") == std::string::npos);
    REQUIRE(result.find("+") != std::string::npos);

    std::filesystem::remove(mask_file);
}

TEST_CASE("Masked maze with sidewinder algorithm", "[masked_maze][sidewinder]")
{
    test_output_dir output_dir{ "masked_maze_sw" };
    const std::string mask_file = output_dir.path.string() + "/test_sw_mask.txt";
    create_simple_mask_file(mask_file);

    REQUIRE(std::filesystem::exists(mask_file));

    REQUIRE(app != nullptr);

    const auto result = app->apply("-m " + mask_file + " -a sidewinder -o stdout");

    REQUIRE_FALSE(result.empty());
    REQUIRE(result.find("Error") == std::string::npos);
    REQUIRE(result.find("+") != std::string::npos);

    std::filesystem::remove(mask_file);
}

TEST_CASE("Masked maze with DFS algorithm", "[masked_maze][dfs]")
{
    test_output_dir output_dir{ "masked_maze_dfs" };
    const std::string mask_file = output_dir.path.string() + "/test_dfs_mask.txt";
    create_simple_mask_file(mask_file);

    REQUIRE(std::filesystem::exists(mask_file));

    REQUIRE(app != nullptr);

    const auto result = app->apply("-m " + mask_file + " -a dfs -o stdout");

    REQUIRE_FALSE(result.empty());
    REQUIRE(result.find("Error") == std::string::npos);
    REQUIRE(result.find("+") != std::string::npos);

    std::filesystem::remove(mask_file);
}

TEST_CASE("Masked maze with distances", "[masked_maze][distances]")
{
    test_output_dir output_dir{ "masked_maze_distances" };
    const std::string mask_file = output_dir.path.string() + "/test_distances_mask.txt";
    create_simple_mask_file(mask_file);

    REQUIRE(std::filesystem::exists(mask_file));

    REQUIRE(app != nullptr);

    const auto result = app->apply("-m " + mask_file + " -a binary_tree -d -o stdout");

    REQUIRE_FALSE(result.empty());
    REQUIRE(result.find("Error") == std::string::npos);

    std::filesystem::remove(mask_file);
}

TEST_CASE("Masked maze with complex pattern", "[masked_maze][complex]")
{
    test_output_dir output_dir{ "masked_maze_complex" };
    const std::string mask_file = output_dir.path.string() + "/test_complex_mask.txt";
    create_complex_mask_file(mask_file);

    REQUIRE(std::filesystem::exists(mask_file));

    REQUIRE(app != nullptr);

    const auto result = app->apply("-m " + mask_file + " -a dfs -o stdout");

    REQUIRE_FALSE(result.empty());
    REQUIRE(result.find("Error") == std::string::npos);
    REQUIRE(result.find("+") != std::string::npos);

    std::filesystem::remove(mask_file);
}

TEST_CASE("Masked maze with seed", "[masked_maze][seed]")
{
    test_output_dir output_dir{ "masked_maze_seed" };
    const std::string mask_file = output_dir.path.string() + "/test_seed_mask.txt";
    create_simple_mask_file(mask_file);

    REQUIRE(std::filesystem::exists(mask_file));
    REQUIRE(app != nullptr);

    const auto result1 = app->apply("-m " + mask_file + " -a binary_tree -s 42 -o stdout");

    REQUIRE_FALSE(result1.empty());
    REQUIRE(result1.find("Error") == std::string::npos);
    INFO("Result1: " << result1);
    REQUIRE(result1.find("+") != std::string::npos);

    std::filesystem::remove(mask_file);
}

TEST_CASE("Masked maze with non-existent file", "[masked_maze][error]")
{
    REQUIRE(app != nullptr);

    const auto result = app->apply("-m nonexistent_mask.txt -a binary_tree -o stdout");

    REQUIRE_FALSE(result.empty());
}

TEST_CASE("Masked maze output to PNG", "[masked_maze][png]")
{
    test_output_dir output_dir{ "masked_maze_png" };
    const std::string mask_file = output_dir.path.string() + "/test_png_mask.txt";
    const std::string output_file = output_dir.path.string() + "/test_masked_output.png";

    create_simple_mask_file(mask_file);

    REQUIRE(std::filesystem::exists(mask_file));

    REQUIRE(app != nullptr);

    const auto result = app->apply("-m " + mask_file + " -a sidewinder -o " + output_file);

    REQUIRE_FALSE(result.empty());

    if (std::filesystem::exists(output_file))
    {
        REQUIRE(std::filesystem::file_size(output_file) > 0);
        std::filesystem::remove(output_file);
    }

    std::filesystem::remove(mask_file);
}

TEST_CASE("Masked maze with long form arguments", "[masked_maze][long_args]")
{
    test_output_dir output_dir{ "masked_maze_long" };
    const std::string mask_file = output_dir.path.string() + "/test_long_args_mask.txt";
    create_simple_mask_file(mask_file);

    REQUIRE(std::filesystem::exists(mask_file));

    REQUIRE(app);

    const auto result = app->apply("--mask=" + mask_file + " --algo=binary_tree --output=stdout");

    REQUIRE_FALSE(result.empty());
    std::filesystem::remove(mask_file);
}
