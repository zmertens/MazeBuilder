#include <MazeBuilder/factory.h>

#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/maze.h>

#if defined(MAZE_DEBUG)
#include <iostream>
#endif

#include <numeric>
#include <algorithm>
#include <vector>

using namespace mazes;

std::optional<std::unique_ptr<maze>> factory::create(configurator const& config) noexcept {
    using namespace std;

    mt19937 mt { config.seed() };
    auto get_int = [&mt](auto low, auto high) {
        uniform_int_distribution<int> dist {low, high};
        return dist(mt);
    };

    auto g = create_grid(cref(config));

    if (!g.has_value()) {
        return nullopt;
    }

    auto callback = [&config, &get_int, &mt, &g]()->bool {
        switch (config._algo()) {
        case algo::BINARY_TREE: {

            static binary_tree bt;

            return bt.run(std::cref(g.value()), std::cref(get_int), std::cref(mt));
        }

        case algo::SIDEWINDER: {

            static sidewinder sw;

            return sw.run(std::ref(g.value()), std::cref(get_int), std::cref(mt));
        }

        case algo::DFS: {

            static dfs d;

            return d.run(std::ref(g.value()), std::cref(get_int), std::cref(mt));
        }

        // Fail on unknown maze type
        default: return false;

        } // switch
        }; // lambda

    auto dimens = g.value()->get_dimensions();

    std::vector<int> shuffled_indices(std::get<0>(dimens) * std::get<1>(dimens));
    std::iota(shuffled_indices.begin(), shuffled_indices.end(), 0);
    std::shuffle(shuffled_indices.begin(), shuffled_indices.end(), mt);

    if (auto grid_ptr = dynamic_cast<grid*>(g.value().get())) {

        grid_ptr->register_observer(cref(callback));

        grid_ptr->start_configuration(shuffled_indices);

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

bool factory::apply_algo_to_grid(configurator const& config, std::unique_ptr<grid_interface> const& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) noexcept {
    
    switch (config._algo()) {
        case algo::BINARY_TREE: {
        
            static binary_tree bt;
        
            return bt.run(std::cref(g), std::cref(get_int), std::cref(rng));
        }
        
        case algo::SIDEWINDER: {
        
            static sidewinder sw;
        
            return sw.run(std::ref(g), std::cref(get_int), std::cref(rng));
        }

        case algo::DFS: {
        
            static dfs d;
        
            return d.run(std::ref(g), std::cref(get_int), std::cref(rng));
        }

        // Fail on unknown maze type
        default: return false;
        
    } // switch
}

