#include <MazeBuilder/factory.h>

#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/maze.h>

using namespace mazes;

std::optional<std::unique_ptr<maze>> factory::create(configurator const& config) noexcept {
    using namespace std;

    mt19937 mt { config.seed() };
    auto get_int = [&mt](auto low, auto high) {
        uniform_int_distribution<int> dist {low, high};
        return dist(mt);
    };

    unique_ptr<grid_interface> g = nullptr;
    if (config.distances()) {
        g = make_unique<distance_grid>(config.rows(), config.columns(), config.levels());
    } else {
        g = make_unique<grid>(config.rows(), config.columns(), config.levels());
    }

    // Call get_future on the specific grid type
    auto success{ false };
    if (auto distance_grid_ptr = dynamic_cast<distance_grid*>(g.get())) {
        success = distance_grid_ptr->get_future().get();
    } else if (auto grid_ptr = dynamic_cast<grid*>(g.get())) {
        success = grid_ptr->get_future().get();
    }

    if (success && apply_algo_to_grid(cref(config), ref(g), cref(get_int), cref(mt))) {
        return make_optional(make_unique<maze>(std::move(g), cref(config)));
    }
    return nullopt;
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

