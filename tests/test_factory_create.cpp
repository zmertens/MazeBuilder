#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid.h>
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

    grid_factory factory1{};

#if defined(MAZE_BENCHMARK)

    BENCHMARK("Benchmark factory::create") {

        [[maybe_unused]]
        auto g = factory1.create(configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO_TO_RUN).seed(SEED));
    };
#else

    SECTION("Create grid with factory - backward compatibility") {

        auto g = factory1.create(configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO_TO_RUN).seed(SEED));
        REQUIRE(g != nullptr);
    }

    SECTION("Create grid with factory - new registration method") {
        
        // Register a custom creator
        factory1.register_creator("test_grid", [](const configurator& config) -> std::unique_ptr<grid_interface> {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
        });

        // Create using the registered key
        auto g = factory1.create("test_grid", configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO_TO_RUN).seed(SEED));
        REQUIRE(g != nullptr);
    }
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


TEST_CASE("Grid factory registration", "[factory registration]") {

    grid_factory factory;

    SECTION("Default creators are registered") {
        auto keys = factory.get_registered_keys();
        REQUIRE(keys.size() >= 3); // At least grid, distance_grid, colored_grid

        REQUIRE(factory.is_registered("grid"));
        REQUIRE(factory.is_registered("distance_grid"));
        REQUIRE(factory.is_registered("colored_grid"));
    }

    SECTION("Can register custom creator") {
        auto custom_creator = [](const configurator& config) -> std::unique_ptr<grid_interface> {
            return std::make_unique<grid>(config.rows() * 2, config.columns() * 2, config.levels());
            };

        REQUIRE(factory.register_creator("custom_grid", custom_creator));
        REQUIRE(factory.is_registered("custom_grid"));

        // Cannot register same key twice
        REQUIRE_FALSE(factory.register_creator("custom_grid", custom_creator));
    }

    SECTION("Can create grid using registered key") {
        configurator config;
        config.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED);

        auto grid = factory.create("grid", config);
        REQUIRE(grid != nullptr);

        auto distance_grid = factory.create("distance_grid", config);
        REQUIRE(distance_grid != nullptr);

        auto colored_grid = factory.create("colored_grid", config);
        REQUIRE(colored_grid != nullptr);
    }

    SECTION("Create returns nullptr for unregistered key") {
        configurator config;
        config.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED);

        auto grid = factory.create("non_existent_key", config);
        REQUIRE(grid == nullptr);
    }

    SECTION("Can unregister creator") {
        auto custom_creator = [](const configurator& config) -> std::unique_ptr<grid_interface> {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
            };

        REQUIRE(factory.register_creator("temp_grid", custom_creator));
        REQUIRE(factory.is_registered("temp_grid"));

        REQUIRE(factory.unregister_creator("temp_grid"));
        REQUIRE_FALSE(factory.is_registered("temp_grid"));

        // Cannot unregister non-existent key
        REQUIRE_FALSE(factory.unregister_creator("temp_grid"));
    }

    SECTION("Backward compatibility - create with config only") {
        configurator config;
        config.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED);

        // Default behavior without distances
        auto grid1 = factory.create(config);
        REQUIRE(grid1 != nullptr);

        // With distances but text output
        config.distances(true);
        auto grid2 = factory.create(config);
        REQUIRE(grid2 != nullptr);

        // With distances and image output
        config.output_id(output::PNG);
        auto grid3 = factory.create(config);
        REQUIRE(grid3 != nullptr);
    }

    SECTION("Clear removes all creators and re-registers defaults") {
        auto custom_creator = [](const configurator& config) -> std::unique_ptr<grid_interface> {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
            };

        factory.register_creator("temp_grid", custom_creator);
        REQUIRE(factory.is_registered("temp_grid"));

        factory.clear();

        REQUIRE_FALSE(factory.is_registered("temp_grid"));
        REQUIRE(factory.is_registered("grid")); // Defaults are re-registered
    }
}



