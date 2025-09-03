#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/maze_factory.h>
#include <MazeBuilder/maze_str.h>
#include <MazeBuilder/progress.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/stringify.h>
#include <MazeBuilder/string_utils.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

using namespace mazes;
using namespace std;

static constexpr auto ROWS = 10, COLUMNS = 5, LEVELS = 1;

static constexpr auto ALGO_DFS = algo::DFS;

static constexpr auto SEED = 12345;

struct pcout : public stringstream
{
    static inline mutex cout_mutex;

    ~pcout()
    {
        lock_guard<mutex> l{cout_mutex};
        cout << rdbuf();
        cout.flush();
    }
};

// Helper function to create maze and return str
static string create(const configurator &config)
{
    auto grid_creator = [](const configurator &config) -> std::unique_ptr<grid_interface>
    {
        return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
    };

    auto maze_creator = [grid_creator](const configurator &config) -> std::unique_ptr<maze_interface>
    {
        if (config.algo_id() != algo::DFS)
        {
            return nullptr;
        }

        grid_factory gf{};

        if (!gf.is_registered("g1"))
        {

            REQUIRE(gf.register_creator("g1", grid_creator));
        }

        if (auto igrid = gf.create("g1", cref(config)); igrid.has_value())
        {
            static dfs _dfs{};

            static randomizer rng{};

            rng.seed(config.seed());

            auto &&igridimpl = igrid.value();

            if (auto success = _dfs.run(igridimpl.get(), ref(rng)))
            {
                static stringify _stringifier;

                _stringifier.run(igridimpl.get(), ref(rng));

                return make_unique<maze_str>(igridimpl->operations().get_str());
            }
        }

        return nullptr;
    };

    string s{};

    auto duration = progress<>::duration([&config, maze_creator, &s]() -> bool
                                         {

        maze_factory mf{};

        if (!mf.is_registered("custom_maze")) {

            REQUIRE(mf.register_creator("custom_maze", maze_creator));
        }

        auto maze_optional = mf.create("custom_maze", cref(config));

        REQUIRE(maze_optional.has_value());

        s = maze_optional.value()->maze();
        
        return !s.empty(); });

    pcout{} << duration.count() << " ms" << endl;

    return s;
}

static string concat(const string &a, const string &b)
{

    return a + b;
}

template <typename F>
static auto asynchronize(F f)
{
    return [f](auto... xs)
    {
        return [=]()
        {
            return async(launch::async, f, xs...);
        };
    };
}

template <typename F>
static auto fut_unwrap(F f)
{
    return [f](auto... xs)
    {
        return f(xs.get()...);
    };
}

template <typename F>
static auto async_adapter(F f)
{
    return [f](auto... xs)
    {
        return [=]()
        {
            return async(launch::async, fut_unwrap(f), xs()...);
        };
    };
}

TEST_CASE("Workflow static checks", "[workflow_static_checks]")
{

    STATIC_REQUIRE(std::is_default_constructible<mazes::grid_factory>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::grid_factory>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_constructible<mazes::grid_factory>::value);
    STATIC_REQUIRE_FALSE(std::is_copy_assignable<mazes::grid_factory>::value);
    STATIC_REQUIRE_FALSE(std::is_move_constructible<mazes::grid_factory>::value);
    STATIC_REQUIRE_FALSE(std::is_move_assignable<mazes::grid_factory>::value);

    STATIC_REQUIRE(std::is_default_constructible<mazes::randomizer>::value);
    STATIC_REQUIRE(std::is_destructible<mazes::randomizer>::value);
    STATIC_REQUIRE(std::is_copy_constructible<mazes::randomizer>::value);
    STATIC_REQUIRE(std::is_copy_assignable<mazes::randomizer>::value);
    STATIC_REQUIRE(std::is_move_constructible<mazes::randomizer>::value);
    STATIC_REQUIRE(std::is_move_assignable<mazes::randomizer>::value);
}

TEST_CASE("Test grid_factory create1", "[create1]")
{

    grid_factory factory1{};

    static const string PRODUCT_NAME_1 = "test_grid";

    // Register a custom creator
    factory1.register_creator(PRODUCT_NAME_1, [](const configurator &config) -> std::unique_ptr<grid_interface>
                              { return std::make_unique<grid>(config.rows(), config.columns(), config.levels()); });

    BENCHMARK("Benchmark grid_factory::create")
    {

        // Create using the registered key
        REQUIRE(factory1.create(PRODUCT_NAME_1, configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO_DFS).seed(SEED)) != std::nullopt);
    };
}

TEST_CASE("Test full workflow", "[full workflow]")
{

    grid_factory g_factory{};

    auto key{"key"};

    g_factory.register_creator(key, [](const configurator &config) -> std::unique_ptr<grid_interface>
                               { return std::make_unique<grid>(config.rows(), config.columns(), config.levels()); });

    auto g = g_factory.create(key, configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO_DFS).seed(SEED));

    randomizer rndmzr{};

    stringify stringifier{};

    REQUIRE(stringifier.run(g.value().get(), rndmzr));

    if (auto casted_grid = dynamic_cast<grid *>(g.value().get()); casted_grid != nullptr)
    {

        REQUIRE_FALSE(casted_grid->operations().get_str().empty());
    }
    else
    {

        FAIL("Failed to cast to grid");
    }
}

TEST_CASE("Test full workflow with large grid", "[full workflow][large]")
{

    grid_factory g_factory{};

    auto key{"key"};

    g_factory.register_creator(key, [](const configurator &config) -> std::unique_ptr<grid_interface>
                               { return std::make_unique<grid>(config.rows(), config.columns(), config.levels()); });

    auto g = g_factory.create(key, configurator()
                                       .rows(configurator::MAX_ROWS)
                                       .columns(configurator::MAX_COLUMNS)
                                       .levels(configurator::MAX_LEVELS)
                                       .algo_id(ALGO_DFS)
                                       .seed(SEED));

    // Verify the grid was created successfully
    REQUIRE(g.has_value());
}

TEST_CASE("Invalid args when converting algo string", "[invalid args]")
{

    vector<string> algos_to_convert = {"dfz", "BINARY_TREE", "adjacentwinder"};

    for (auto a : algos_to_convert)
    {
        REQUIRE_THROWS_AS(mazes::to_algo_from_string(cref(a)), std::invalid_argument);
    }
}

TEST_CASE("randomizer::get_num_ints generates correct number of integers", "[randomizer]")
{
    randomizer rng;
    rng.seed(SEED);

    static constexpr auto low = 0, high = 10;

    SECTION("Validate random number values are within specific range")
    {

        auto result = rng.get_vector_ints(low, high - 1, high);
        REQUIRE(result.size() == high);
        for (int num : result)
        {
            REQUIRE(num >= low);
            REQUIRE(num <= high);
        }
    }

    SECTION("Generate all integers in a range")
    {
        auto result = rng.get_vector_ints(low, high, 2);
        REQUIRE(result.size() == 2);
        std::sort(result.begin(), result.end());
        REQUIRE(result.cend() != result.cbegin());
    }

    SECTION("Empty range [high, low]")
    {
        auto result = rng.get_vector_ints(high, low);
        REQUIRE(result.empty());
    }

    SECTION("Zero integers requested")
    {
        auto result = rng.get_vector_ints(0, -1);
        REQUIRE(result.empty());
    }
}

TEST_CASE("Grid grid_factory registration", "[grid_factory registration]")
{

    grid_factory grid_factory{};

    SECTION("Can register custom creator")
    {
        auto custom_creator = [](const configurator &config) -> std::unique_ptr<grid_interface>
        {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
        };

        REQUIRE(grid_factory.register_creator("custom_grid", custom_creator));
        REQUIRE(grid_factory.is_registered("custom_grid"));

        // Cannot register same key twice
        REQUIRE_FALSE(grid_factory.register_creator("custom_grid", custom_creator));
    }

    SECTION("Can register custom creator with distances")
    {
        auto custom_creator = [](const auto &config) -> auto
        {
            return std::make_unique<distance_grid>(config.rows(), config.columns(), config.levels());
        };

        REQUIRE(grid_factory.register_creator("distance_grid", custom_creator));
        REQUIRE(grid_factory.is_registered("distance_grid"));

        // Cannot register same key twice
        REQUIRE_FALSE(grid_factory.register_creator("distance_grid", custom_creator));
    }

    SECTION("Can create grid using registered key")
    {
        configurator config;
        config.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED);

        auto grid = grid_factory.create("grid", config);
        REQUIRE(grid != nullptr);

        auto distance_grid = grid_factory.create("distance_grid", config);
        REQUIRE(distance_grid != nullptr);

        auto colored_grid = grid_factory.create("colored_grid", config);
        REQUIRE(colored_grid != nullptr);
    }

    SECTION("Create returns nullptr for unregistered key")
    {
        configurator config;
        config.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED);

        auto grid = grid_factory.create("non_existent_key", config);
        REQUIRE_FALSE(grid.has_value());
    }

    SECTION("Can unregister creator")
    {
        auto custom_creator = [](const configurator &config) -> std::unique_ptr<grid_interface>
        {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
        };

        REQUIRE(grid_factory.register_creator("temp_grid", custom_creator));
        REQUIRE(grid_factory.is_registered("temp_grid"));

        REQUIRE(grid_factory.unregister_creator("temp_grid"));
        REQUIRE_FALSE(grid_factory.is_registered("temp_grid"));

        // Cannot unregister non-existent key
        REQUIRE_FALSE(grid_factory.unregister_creator("temp_grid"));
    }

    SECTION("Backward compatibility - create with config only")
    {
        configurator config;
        config.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED);

        // Default behavior without distances
        auto grid1 = grid_factory.create("test", config);
        REQUIRE(grid1 != nullptr);

        // With distances but text output
        config.distances(true);
        auto grid2 = grid_factory.create("test", config);
        REQUIRE(grid2 != nullptr);

        // With distances and image output
        config.output_format_id(output_format::PNG);
        auto grid3 = grid_factory.create("test", config);
        REQUIRE(grid3 != nullptr);
    }

    SECTION("Clear removes all creators")
    {
        auto custom_creator = [](const configurator &config) -> std::unique_ptr<grid_interface>
        {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
        };

        grid_factory.register_creator("temp_grid", custom_creator);
        REQUIRE(grid_factory.is_registered("temp_grid"));

        grid_factory.clear();

        REQUIRE_FALSE(grid_factory.is_registered("temp_grid"));
    }
}

// Source material here
// https://github.com/PacktPublishing/Cpp17-STL-Cookbook/blob/master/Chapter09/chains.cpp
TEST_CASE("Maze maze_factory registration with async", "[maze_factory async workflow]")
{
    configurator config1{};
    config1.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED).distances(true).algo_id(ALGO_DFS);

    configurator config2{};
    config2.rows(COLUMNS).columns(ROWS).levels(LEVELS).seed(SEED).distances(true).algo_id(ALGO_DFS);

    auto pcreate(asynchronize(create));
    auto pconcat(async_adapter(concat));

    auto result = pconcat(pcreate(cref(config2)), pconcat(pcreate(cref(config1)), pcreate(cref(config2))));

    pcout{} << "Setup done. Nothing executed yet.\n";

    // Use structured bindings to capture both the maze and timing
    auto [content, execution_time] = [&result]()
    {
        std::string maze_content;

        auto duration = progress<>::duration([&result, &maze_content]() -> bool
                                             {
                                                 maze_content = result().get();
                                                 return !maze_content.empty();
                                             });

        return std::make_tuple(std::move(maze_content), duration.count());
    }();

    REQUIRE_FALSE(content.empty());

    pcout{} << "Async execution time: " << execution_time << " ms\n";
}
