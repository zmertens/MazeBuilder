#include <MazeBuilder/factory.h>

#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/cell_factory.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/maze.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/stringify.h>

#include <functional>
#include <iostream>
#include <sstream>
#include <vector>

using namespace mazes;

/// @brief 
/// @param rows 10
/// @param columns 10
/// @return 
std::optional<std::unique_ptr<maze>> factory::create_with_rows_columns(unsigned int rows, unsigned int columns) noexcept {
    using namespace std;

    configurator config;
    config.rows(rows).columns(columns).levels(1).distances(false).seed(0).algo_id(algo::DFS);
    config.output_id(output::STDOUT).block_id(-1);

    return create(config);
}

std::unique_ptr<maze> factory::create(configurator const& config) noexcept {
    using namespace std;

    // Create the grid
    auto g = create_grid(cref(config));
    if (!g.has_value()) {
        return nullptr;
    }

    // Set up randomizer
    randomizer rng;
    rng.seed(config.seed());
    auto shared_rng = make_shared<randomizer>(rng);

    // Get dimensions from grid
    const auto& ops = g.value()->operations();
    auto dimensions = ops.get_dimensions();

    // Create cells using the cell_factory
    static cell_factory cell_maker;
    auto cells = cell_maker.create_cells(dimensions);
    
    // Get random indices for configuration
    auto random_ints = rng.get_num_ints_incl(0, config.rows() * config.columns() - 1);
    
    // Configure cells with the cell_factory
    cell_maker.configure(cells, dimensions, random_ints);
    
    // Set the configured cells in the grid
    g.value()->operations().set_cells(cells);

    // Run algorithm on the grid
    auto success = false;
    switch (config.algo_id()) {
    case algo::BINARY_TREE: {
        static binary_tree bt;
        success = bt.run(cref(g.value()), std::ref(*shared_rng));
        break;
    }
    case algo::SIDEWINDER: {
        static sidewinder sw;
        success = sw.run(cref(g.value()), std::ref(*shared_rng));
        break;
    }
    case algo::DFS: {
        static dfs d;
        success = d.run(cref(g.value()), std::ref(*shared_rng));
        break;
    }
    default:
        return nullptr;
    }

    // Calculate distances if needed
    if (success && config.distances()) {
        // Use operations to get a distance grid and calculate distances
        if (auto dist_grid = dynamic_cast<distance_grid*>(g.value().get())) {
            dist_grid->calculate_distances(ops.num_cells() - 1, 0);
        }
    }

    // Get string representation of the maze
    string maze_str;
    if (success) {
        // Use stringz::stringify instead of the broken stringify class
        auto temp_maze = make_unique<maze>(cref(config), "");
        mazes::stringify str_tool;
        success = str_tool.run(g.value(), *shared_rng);
        maze_str = g.value()->operations().get_str();
    }

    // Clean up and create the maze
    g.value()->operations().clear_cells();
    
    unique_ptr<maze> result = nullptr;
    if (success) {
        result = make_unique<maze>(cref(config), maze_str);
    }

    if (!success) {

        cerr << "Failed to create maze with the given configuration." << endl;

        return nullptr;
    }
    
    return result;
}

std::optional<std::unique_ptr<grid_interface>> factory::create_grid(configurator const& config) noexcept {
    using namespace std;

    unique_ptr<grid_interface> g = nullptr;

    if (config.distances()) {
        if (config.output_id() == output::PNG || config.output_id() == output::JPEG) {

            g = make_unique<colored_grid>(config.rows(), config.columns(), config.levels());
        } else {

            g = make_unique<distance_grid>(config.rows(), config.columns(), config.levels());
        }
    } else {
        g = make_unique<grid>(config.rows(), config.columns(), config.levels());
    }

    return make_optional(std::move(g));
}

