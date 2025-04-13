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

#include <vector>

using namespace mazes;

/// @brief 
/// @param rows 10
/// @param columns 10
/// @return 
std::optional<std::unique_ptr<maze>> factory::create_q(unsigned int rows, unsigned int columns) noexcept {
    using namespace std;

    configurator config;
    config.rows(rows).columns(columns).levels(1).distances(false).seed(0)._algo(algo::DFS);
    config._output(output::STDOUT).block_id(-1);

    return create(config);
}

std::optional<std::unique_ptr<maze>> factory::create(configurator const& config) noexcept {
    using namespace std;

    auto g = create_grid(cref(config));

    if (!g.has_value()) {
        return nullopt;
    }

    // Create a random number generator
    randomizer rng;
    rng.seed(config.seed());

    auto callback = [&config, &rng, &g]()->bool {
        switch (config._algo()) {
        case algo::BINARY_TREE: {

            static binary_tree bt;

            return bt.run(std::cref(g.value()), std::ref(rng));
        }

        case algo::SIDEWINDER: {

            static sidewinder sw;

            return sw.run(std::ref(g.value()), std::ref(rng));
        }

        case algo::DFS: {

            static dfs d;

            return d.run(std::ref(g.value()), std::ref(rng));
        }

        // Fail on unknown maze type
        default: return false;

        } // switch
        }; // lambda

    auto random_ints = rng.get_num_ints_incl(0, config.rows() * config.columns());

    if (auto grid_ptr = dynamic_cast<grid*>(g.value().get())) {

        grid_ptr->register_observer(cref(callback));

        grid_ptr->start_configuration(cref(random_ints));

        if (grid_ptr->is_observed()) {

            return make_optional(make_unique<maze>(std::move(g.value()), cref(config)));
        }
    }

    return nullopt;
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

