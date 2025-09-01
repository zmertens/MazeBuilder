#include <catch2/catch_test_macros.hpp>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/stringify.h>

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

using namespace mazes;
using namespace std;

static constexpr auto ROWS = 10, COLUMNS = 5, LEVELS = 1;

static constexpr auto ALGO_TO_RUN = algo::DFS;

static constexpr auto SEED = 12345;

TEST_CASE("Workflow static checks", "[workflow_static_checks]") {

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

TEST_CASE( "Test grid_factory create1", "[create1]" ) {

    grid_factory factory1{};

#if defined(MAZE_BENCHMARK)

    BENCHMARK("Benchmark grid_factory::create") {

        [[maybe_unused]]
        auto g = factory1.create("unused", configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO_TO_RUN).seed(SEED));
    };
#else

    SECTION("Create grid with grid_factory - new registration method") {

        static const string PRODUCT_NAME_1 = "test_grid";
        
        // Register a custom creator
        factory1.register_creator(PRODUCT_NAME_1, [](const configurator& config) -> std::unique_ptr<grid_interface> {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
        });

        // Create using the registered key
        auto g = factory1.create(PRODUCT_NAME_1, configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO_TO_RUN).seed(SEED));
        REQUIRE(g != nullptr);
    }
#endif
}

TEST_CASE("Test full workflow", "[full workflow]") {

    grid_factory g_factory{};

    auto key{"key"};

    g_factory.register_creator(key, [](const configurator& config) -> std::unique_ptr<grid_interface> {

        return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
    });

    auto g = g_factory.create(key, configurator().rows(ROWS).columns(COLUMNS).levels(LEVELS).algo_id(ALGO_TO_RUN).seed(SEED));

    randomizer rndmzr{};

    stringify stringifier{};

    REQUIRE(stringifier.run(g.value().get(), rndmzr));

    if (auto casted_grid = dynamic_cast<grid*>(g.value().get()); casted_grid != nullptr) {

        cout << casted_grid->operations().get_str() << endl;

        REQUIRE_FALSE(casted_grid->operations().get_str().empty());
    } else {

        FAIL("Failed to cast to grid");
    }
    
}

TEST_CASE("Test full workflow with large grid", "[full workflow][large]") {

    grid_factory g_factory{};

    auto key{"key"};

    g_factory.register_creator(key, [](const configurator& config) -> std::unique_ptr<grid_interface> {

        return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
    });

    auto g = g_factory.create(key, configurator()
        .rows(configurator::MAX_ROWS)
        .columns(configurator::MAX_COLUMNS)
        .levels(configurator::MAX_LEVELS)
        .algo_id(ALGO_TO_RUN)
        .seed(SEED));

    // Verify the grid was created successfully
    REQUIRE(g.has_value());
    
    // Verify dimensions are correct
    auto [rows, cols, levels] = g.value()->operations().get_dimensions();
    REQUIRE(rows == configurator::MAX_ROWS);
    REQUIRE(cols == configurator::MAX_COLUMNS);
    REQUIRE(levels == configurator::MAX_LEVELS);
    
    // With lazy evaluation, initially no cells should be created
    REQUIRE(g.value()->operations().num_cells() == 0);
    
    // Access a specific cell should create it lazily
    auto test_cell = g.value()->operations().search(1000);
    REQUIRE(test_cell != nullptr);
    REQUIRE(test_cell->get_index() == 1000);
    
    // Now we should have at least one cell
    REQUIRE(g.value()->operations().num_cells() > 0);
    REQUIRE(g.value()->operations().num_cells() < 100000); // Much less than total possible

    randomizer rndmzr{};

    stringify stringifier{};

    // Test memory boundaries within stringify - should fail due to size limit
    REQUIRE_FALSE(stringifier.run(g.value().get(), rndmzr));

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

        auto result = rng.get_vector_ints(low, high - 1, high);
        REQUIRE(result.size() == high);
        for (int num : result) {
            REQUIRE(num >= low);
            REQUIRE(num <= high);
        }
    }

    SECTION("Generate all integers in a range") {
        auto result = rng.get_vector_ints(low, high, 2);
        REQUIRE(result.size() == 2);
        std::sort(result.begin(), result.end());
        REQUIRE(result.cend() != result.cbegin());
    }

    SECTION("Empty range [high, low]") {
        auto result = rng.get_vector_ints(high, low);
        REQUIRE(result.empty());
    }

    SECTION("Zero integers requested") {
        auto result = rng.get_vector_ints(0, -1);
        REQUIRE(result.empty());
    }
}


TEST_CASE("Grid grid_factory registration", "[grid_factory registration]") {

    grid_factory grid_factory{};

    SECTION("Can register custom creator") {
        auto custom_creator = [](const configurator& config) -> std::unique_ptr<grid_interface> {

            return std::make_unique<grid>(config.rows() * 2, config.columns() * 2, config.levels());
            };

        REQUIRE(grid_factory.register_creator("custom_grid", custom_creator));
        REQUIRE(grid_factory.is_registered("custom_grid"));

        // Cannot register same key twice
        REQUIRE_FALSE(grid_factory.register_creator("custom_grid", custom_creator));
    }

    SECTION("Can register custom creator with distances") {
        auto custom_creator = [](const auto& config) -> auto {

            return std::make_unique<distance_grid>(config.rows() * 2, config.columns() * 2, config.levels());
            };

        REQUIRE(grid_factory.register_creator("distance_grid", custom_creator));
        REQUIRE(grid_factory.is_registered("distance_grid"));

        // Cannot register same key twice
        REQUIRE_FALSE(grid_factory.register_creator("distance_grid", custom_creator));
    }

    SECTION("Can create grid using registered key") {
        configurator config;
        config.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED);

        auto grid = grid_factory.create("grid", config);
        REQUIRE(grid != nullptr);

        auto distance_grid = grid_factory.create("distance_grid", config);
        REQUIRE(distance_grid != nullptr);

        auto colored_grid = grid_factory.create("colored_grid", config);
        REQUIRE(colored_grid != nullptr);
    }

    SECTION("Create returns nullptr for unregistered key") {
        configurator config;
        config.rows(ROWS).columns(COLUMNS).levels(LEVELS).seed(SEED);

        auto grid = grid_factory.create("non_existent_key", config);
        REQUIRE_FALSE(grid.has_value());
    }

    SECTION("Can unregister creator") {
        auto custom_creator = [](const configurator& config) -> std::unique_ptr<grid_interface> {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
            };

        REQUIRE(grid_factory.register_creator("temp_grid", custom_creator));
        REQUIRE(grid_factory.is_registered("temp_grid"));

        REQUIRE(grid_factory.unregister_creator("temp_grid"));
        REQUIRE_FALSE(grid_factory.is_registered("temp_grid"));

        // Cannot unregister non-existent key
        REQUIRE_FALSE(grid_factory.unregister_creator("temp_grid"));
    }

    SECTION("Backward compatibility - create with config only") {
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

    SECTION("Clear removes all creators") {
        auto custom_creator = [](const configurator& config) -> std::unique_ptr<grid_interface> {
            return std::make_unique<grid>(config.rows(), config.columns(), config.levels());
            };

        grid_factory.register_creator("temp_grid", custom_creator);
        REQUIRE(grid_factory.is_registered("temp_grid"));

        grid_factory.clear();

        REQUIRE_FALSE(grid_factory.is_registered("temp_grid"));
    }
}



