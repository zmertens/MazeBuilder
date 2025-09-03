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

static constexpr auto ALGO_TO_RUN = algo::DFS;

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

static string create(const char *s)
{
    pcout{} << "3s CREATE " << quoted(s) << '\n';
    this_thread::sleep_for(3s);
    return {s};
}

static string concat(const string &a, const string &b)
{
    pcout{} << "5s CONCAT "
            << quoted(a) << " "
            << quoted(b) << '\n';
    this_thread::sleep_for(5s);
    return a + b;
}

static string twice(const string &s)
{

    pcout{} << "3s TWICE  " << quoted(s) << '\n';
    this_thread::sleep_for(3s);
    return s + s;
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
        REQUIRE(factory1.create(PRODUCT_NAME_1, configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO_TO_RUN).seed(SEED)) != std::nullopt);
    };
}

TEST_CASE("Test full workflow", "[full workflow]")
{

    grid_factory g_factory{};

    auto key{"key"};

    g_factory.register_creator(key, [](const configurator &config) -> std::unique_ptr<grid_interface>
                               { return std::make_unique<grid>(config.rows(), config.columns(), config.levels()); });

    auto g = g_factory.create(key, configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO_TO_RUN).seed(SEED));

    randomizer rndmzr{};

    stringify stringifier{};

    REQUIRE(stringifier.run(g.value().get(), rndmzr));

    if (auto casted_grid = dynamic_cast<grid *>(g.value().get()); casted_grid != nullptr)
    {

        REQUIRE_FALSE(casted_grid->operations().get_str().empty());

        cout << string_utils::format("{}", casted_grid->operations().get_str()).substr(0, 30) << endl;
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
                                       .algo_id(ALGO_TO_RUN)
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

TEST_CASE("Maze maze_factory registration", "[maze_factory workflow]")
{

    maze_factory maze_factory{};

    configurator config;
    config.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED).distances(true);

    auto maze_creator = [](const configurator &config) -> std::unique_ptr<maze_interface>
    {
        grid_factory gf{};

        auto grid_creator = [](const configurator &config) -> std::unique_ptr<grid_interface>
        {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
        };

        REQUIRE(gf.register_creator("g1", grid_creator));

        if (auto igrid = gf.create("g1", config); igrid.has_value())
        {

            dfs _dfs{};

            randomizer rng{};

            rng.seed(config.seed());

            auto &&igridimpl = igrid.value();

            if (auto success = _dfs.run(igridimpl.get(), ref(rng)))
            {

                stringify _stringifier;

                _stringifier.run(igridimpl.get(), ref(rng));

                return make_unique<maze_str>(igridimpl->operations().get_str());
            }
        }

        return nullptr;
    };

    REQUIRE(maze_factory.register_creator("custom_maze", maze_creator));
    REQUIRE(maze_factory.is_registered("custom_maze"));

    // Cannot register same key twice
    REQUIRE_FALSE(maze_factory.register_creator("custom_maze", maze_creator));

    cout << maze_factory.create("custom_maze", config).value()->maze() << endl;
}

// Source material here
// https://github.com/PacktPublishing/Cpp17-STL-Cookbook/blob/master/Chapter09/chains.cpp
TEST_CASE("Test async unwrapping", "[async unwrap]")
{
    auto pcreate(asynchronize(create));
    auto pconcat(async_adapter(concat));
    auto ptwice(async_adapter(twice));

    auto result(
        pconcat(
            ptwice(
                pconcat(
                    pcreate("foo "),
                    pcreate("bar "))),
            pconcat(
                pcreate("this "),
                pcreate("that "))));

    cout << "Setup done. Nothing executed yet.\n";

    cout << result().get() << '\n';
}
