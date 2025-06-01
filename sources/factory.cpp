#include <MazeBuilder/factory.h>

#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/maze.h>
#include <MazeBuilder/randomizer.h>

#if defined(MAZE_DEBUG)
#include <iostream>
#endif

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
    config.rows(rows).columns(columns).levels(1).distances(false).seed(0)._algo(algo::DFS);
    config._output(output::STDOUT).block_id(-1);

    return create(config);
}

std::unique_ptr<maze> factory::create(configurator const& config) noexcept {
    using namespace std;

    auto g = create_grid(cref(config));

    if (!g.has_value()) {
        return nullptr;
    }

    // Create weak references from shared_ptr's
    randomizer rng;
    rng.seed(config.seed());
    auto shared_rng = make_shared<randomizer>(rng);
    auto shared_config = make_shared<const configurator>(config);

    // Get the raw pointer but keep ownership in g
    auto* grid_ptr = dynamic_cast<grid*>(g.value().get());
    if (!grid_ptr) {
        return nullptr;
    }

    auto random_ints = rng.get_num_ints_incl(0, config.rows() * config.columns());
    grid_ptr->configure(cref(random_ints));

    auto success = false;

    switch (shared_config->_algo()) {
    // Algorithm implementations...
    case algo::BINARY_TREE: {
        static binary_tree bt;
        success = bt.run(grid_ptr, std::ref(*shared_rng));
        break;
    }
    case algo::SIDEWINDER: {
        static sidewinder sw;
        success = sw.run(grid_ptr, std::ref(*shared_rng));
        break;
    }
    case algo::DFS: {
        static dfs d;
        success = d.run(grid_ptr, std::ref(*shared_rng));
        break;
    }
    default:
        return nullptr;
    }

    // If the grid is a distance grid, calculate distances
    if (auto distance_grid_ptr = dynamic_cast<distance_grid*>(grid_ptr); success) {
        distance_grid_ptr->calculate_distances(config.rows() * config.columns(), 0);
    }

    std::unique_ptr<maze> result = nullptr;
    if (success) {
        // Create the maze using the grid and config
        stringstream ss;
        ss << *(g.value().get());
        result = make_unique<maze>(cref(config), ss.str());
    }

    // Explicitly clean up the grid before returning
    if (grid_ptr) {
        grid_ptr->clear_cells();
    }

    return result;
}

std::optional<std::unique_ptr<grid_interface>> factory::create_grid(configurator const& config) noexcept {
    using namespace std;

    unique_ptr<grid_interface> g = nullptr;

    if (config.distances()) {
        if (config._output() == output::PNG || config._output() == output::JPEG) {

            g = make_unique<colored_grid>(config.rows(), config.columns(), config.levels());
        } else {

            g = make_unique<distance_grid>(config.rows(), config.columns(), config.levels());
        }
    } else {
        g = make_unique<grid>(config.rows(), config.columns(), config.levels());
    }

    return make_optional(std::move(g));
}

