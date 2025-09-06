#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <functional>
#include <string>
#include <vector>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create.h>
#include <MazeBuilder/create2.h>

TEST_CASE("Create with single configurator", "[create_single]")
{
    mazes::configurator config;
    config.rows(5).columns(5).algo_id(mazes::algo::DFS);
    
    std::string result = mazes::create(config);
    
    REQUIRE_FALSE(result.empty());
}

TEST_CASE("Create with multiple configurators using variadic args", "[create_variadic]")
{
    auto results = mazes::create(
        mazes::configurator().rows(10).columns(10).algo_id(mazes::algo::DFS),
        mazes::configurator().rows(5).columns(5).algo_id(mazes::algo::DFS),
        mazes::configurator().rows(8).columns(6).algo_id(mazes::algo::DFS)
    );
    
    REQUIRE(results.size() == 3);
    for (const auto& maze : results) {
        REQUIRE_FALSE(maze.empty());
    }
}

TEST_CASE("Create with different seed configurators", "[create_seeds]")
{
    auto results = mazes::create(
        mazes::configurator().rows(5).columns(5).algo_id(mazes::algo::DFS).seed(123),
        mazes::configurator().rows(5).columns(5).algo_id(mazes::algo::DFS).seed(456),
        mazes::configurator().rows(5).columns(5).algo_id(mazes::algo::DFS).seed(789)
    );
    
    REQUIRE(results.size() == 3);
    
    // All mazes should be generated
    for (const auto& maze : results) {
        REQUIRE_FALSE(maze.empty());
    }
    
    // Different seeds should produce different mazes
    REQUIRE(results[0] != results[1]);
    REQUIRE(results[1] != results[2]);
    REQUIRE(results[0] != results[2]);
}

TEST_CASE("Create with varied size configurators", "[create_sizes]")
{
    auto results = mazes::create(
        mazes::configurator().rows(3).columns(3).algo_id(mazes::algo::DFS),
        mazes::configurator().rows(10).columns(15).algo_id(mazes::algo::DFS),
        mazes::configurator().rows(7).columns(7).levels(2).algo_id(mazes::algo::DFS)
    );
    
    REQUIRE(results.size() == 3);
    for (const auto& maze : results) {
        REQUIRE_FALSE(maze.empty());
    }
}

TEST_CASE("Create with two configurators", "[create_two]")
{
    auto results = mazes::create(
        mazes::configurator().rows(4).columns(4).algo_id(mazes::algo::DFS),
        mazes::configurator().rows(6).columns(6).algo_id(mazes::algo::DFS)
    );
    
    REQUIRE(results.size() == 2);
    REQUIRE_FALSE(results[0].empty());
    REQUIRE_FALSE(results[1].empty());
}

// Randomizer performs different on different OS's
#if defined(__UNIX__)

TEST_CASE("Create reproducible with same seed", "[create_reproducible]")
{
    // Create the same configuration twice with same seed
    auto results1 = mazes::create(
        mazes::configurator().rows(5).columns(5).algo_id(mazes::algo::DFS).seed(42),
        mazes::configurator().rows(3).columns(3).algo_id(mazes::algo::DFS).seed(99)
    );
    
    auto results2 = mazes::create(
        mazes::configurator().rows(5).columns(5).algo_id(mazes::algo::DFS).seed(42),
        mazes::configurator().rows(3).columns(3).algo_id(mazes::algo::DFS).seed(99)
    );
    
    REQUIRE(results1.size() == 2);
    REQUIRE(results2.size() == 2);
    REQUIRE(results1[0] == results2[0]);
    REQUIRE(results1[1] == results2[1]);
}

#endif // UNIX

TEST_CASE("Create with reference wrapper support", "[create_reference_wrapper]")
{
    mazes::configurator config1;
    config1.rows(5).columns(5).algo_id(mazes::algo::DFS).seed(123);
    
    mazes::configurator config2;
    config2.rows(3).columns(3).algo_id(mazes::algo::DFS).seed(456);
    
    // Test single reference wrapper
    std::string single_result = mazes::create(std::cref(config1));
    REQUIRE_FALSE(single_result.empty());
    
    // Test multiple reference wrappers
    auto results = mazes::create(std::cref(config1), std::cref(config2));
    REQUIRE(results.size() == 2);
    REQUIRE_FALSE(results[0].empty());
    REQUIRE_FALSE(results[1].empty());
}

#if defined(MAZE_BENCHMARK)

TEST_CASE("Create mazes and benchmark", "[create workflow]")
{
    static constexpr auto ROWS{28};
    static constexpr auto COLUMNS{59};

    BENCHMARK("Create Binary Tree")
    {
        auto result = mazes::create(mazes::configurator().rows(ROWS).columns(COLUMNS).algo_id(mazes::algo::BINARY_TREE));
        REQUIRE_FALSE(result.empty());
    };

    BENCHMARK("Create DFS")
    {
        auto result = mazes::create(mazes::configurator().rows(ROWS).columns(COLUMNS).algo_id(mazes::algo::DFS));
        REQUIRE_FALSE(result.empty());
    };

    BENCHMARK("Create Sidewinder")
    {
        auto result = mazes::create(mazes::configurator().rows(ROWS).columns(COLUMNS).algo_id(mazes::algo::SIDEWINDER));
        REQUIRE_FALSE(result.empty());
    };

    BENCHMARK("Create Sidewinder with create2")
    {
        std::vector<mazes::configurator> configs;
        configs.emplace_back(mazes::configurator().rows(ROWS).columns(COLUMNS).algo_id(mazes::algo::SIDEWINDER));
        auto result = mazes::create2(std::cref(configs));
        REQUIRE_FALSE(result.empty());
    };
}

#endif // MAZE_BENCHMARK
