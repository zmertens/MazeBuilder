#include <MazeBuilder/factory.h>

#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/maze.h>

using namespace mazes;

std::optional<std::unique_ptr<maze>> factory::create(configurator const& config) noexcept {
    using namespace std;

    mt19937 mt { config.seed() };
    auto get_int = [&mt](auto low, auto high) {
        uniform_int_distribution<int> dist {low, high};
        return dist(mt);
    };

    unique_ptr<grid_interface> g = make_unique<grid>(config.rows(), config.columns(), config.levels());
    if (run_algo_on_grid(cref(config), ref(g), cref(get_int), cref(mt))) {
        return make_optional(make_unique<maze>(std::move(g)));
    }
    return nullopt;
}

bool factory::run_algo_on_grid(configurator const& config, std::unique_ptr<grid_interface> const& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) noexcept {
    
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

