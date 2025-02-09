#include <MazeBuilder/factory.h>

#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/maze.h>

using namespace mazes;


std::optional<std::unique_ptr<maze>> factory::create(unsigned int rows, unsigned int columns, unsigned int height) noexcept {
    using namespace std;

    algos a = algos::BINARY_TREE;

    std::mt19937 mt;
    auto get_int = [&mt](auto low, auto high) {
        std::uniform_int_distribution<int> dist {low, high};
        return dist(mt);
    };

    unique_ptr<grid_interface> g = make_unique<grid>(rows, columns, height);
    if (run_algo_on_grid(a, ref(g), get_int, mt)) {
        return make_optional(make_unique<maze>(g));
    } else {
        return nullopt;
    } 
}
	
std::optional<std::unique_ptr<maze>> factory::create(std::tuple<unsigned int, unsigned int, unsigned int> dimensions) noexcept {
    using namespace std;

    algos a = algos::BINARY_TREE;

    std::mt19937 mt;
    auto get_int = [&mt](auto low, auto high) {
        std::uniform_int_distribution<int> dist {low, high};
        return dist(mt);
    };

    auto [rows, columns, height] = dimensions;

    unique_ptr<grid_interface> g = make_unique<grid>(rows, columns, height);
    if (run_algo_on_grid(a, ref(g), get_int, mt)) {
        return make_optional(make_unique<maze>(g));
    } else {
        return nullopt;
    }
}

std::optional<std::unique_ptr<maze>> factory::create(std::tuple<unsigned int, unsigned int, unsigned int> dimensions,
    algos a) noexcept {

    using namespace std;

    std::mt19937 mt;
    auto get_int = [&mt](auto low, auto high) {
        std::uniform_int_distribution<int> dist {low, high};
        return dist(mt);
    };

    auto [rows, columns, height] = dimensions;

    unique_ptr<grid_interface> g = make_unique<grid>(rows, columns, height);
    if (run_algo_on_grid(a, ref(g), get_int, mt)) {
        return make_optional(make_unique<maze>(g));
    } else {
        return nullopt;
    } 

}

std::optional<std::unique_ptr<maze>> factory::create(std::tuple<unsigned int, unsigned int, unsigned int> dimensions,
    algos a, const std::function<int(int, int)>& get_int, const std::mt19937& rng) noexcept {

        using namespace std;

        auto [rows, columns, height] = dimensions;
    
        unique_ptr<grid_interface> g = make_unique<grid>(rows, columns, height);
        if (run_algo_on_grid(a, ref(g), cref(get_int), cref(rng))) {
            return make_optional(make_unique<maze>(g));
        } else {
            return nullopt;
        }     

}

bool factory::run_algo_on_grid(algos a, std::unique_ptr<grid_interface>& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) noexcept {
    
    switch (a) {
        case algos::BINARY_TREE: {
        
            static binary_tree bt;
        
            return bt.run(std::ref(g), std::cref(get_int), std::cref(rng));
        }
        
        case algos::SIDEWINDER: {
        
            static sidewinder sw;
        
            return sw.run(std::ref(g), std::cref(get_int), std::cref(rng));
        }

        case algos::DFS: {
        
            static dfs d;
        
            return d.run(std::ref(g), std::cref(get_int), std::cref(rng));
        }

        // Fail on unknown maze type
        default: return false;
        
    } // switch
}
