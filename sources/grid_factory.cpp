#include <MazeBuilder/grid_factory.h>

#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/stringify.h>

#include <functional>
#include <iostream>
#include <sstream>
#include <vector>

using namespace mazes;

std::unique_ptr<grid_interface> grid_factory::create(configurator const& config) const noexcept {

    using namespace std;

    auto g = create_grid(cref(config));

    if (!g.has_value()) {

        return nullptr;
    }

    // Set up randomizer
    randomizer rng;
    rng.seed(config.seed());

    // Get dimensions from grid
    auto&& ops = g.value()->operations();

    // Generate indices for configuration
    auto indices = rng.get_num_ints_incl(0, config.rows() * config.columns() - 1);

    // Prepare cells
    vector<shared_ptr<cell>> cells_to_set;
    cells_to_set.reserve(config.rows() * config.columns());

    lab::set_neighbors(cref(config), cref(indices), ref(cells_to_set));

    // Set the configured cells in the grid
    ops.set_cells(cref(cells_to_set));

    return std::move(g.value());
}

std::optional<std::unique_ptr<grid_interface>> grid_factory::create_grid(configurator const& config) noexcept {
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

