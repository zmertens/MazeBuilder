#include <random>
#include <exception>
#include <cstdio>
#include <iostream>

#include "craft.h"
#include "grid.h"
#include "binary_tree.h"
#include "sidewinder.h"
#include "args_builder.h"
#include "maze_types_enum.h"

int main(int argc, char* argv[]) {
    using namespace std;

    static constexpr auto MAZE_BUILDER_VERSION = "maze_builder=[2.0.1]";

    static const std::string HELP_MSG = R"help(
        Maze Builder Usages:
          1. ./maze_builder > default_maze.txt
          2. ./maze_builder --seed=1337 --algo=binary_tree -o bt.txt
          3. ./maze_builder -i -s 1337
    )help";

    try {
        mazes::args_builder args (MAZE_BUILDER_VERSION, HELP_MSG, argc, argv);
        mazes::args_state state_of_args {args.get_state()};
        auto&& args_map {args.build()};

#if defined(DEBUGGING)
        std::cout << args << std::endl;
#endif

        if (state_of_args == mazes::args_state::JUST_NEEDS_HELP) {
            cout << HELP_MSG << endl;
            return EXIT_SUCCESS;
        } else if (state_of_args == mazes::args_state::JUST_NEEDS_VERSION) {
            cout << MAZE_BUILDER_VERSION << endl;
            return EXIT_SUCCESS;
        }

        auto use_this_for_seed = args_map.at("seed");
        auto get_int = [](int low, int high) -> int {
            using namespace std;
            random_device rd;
            seed_seq seed {rd()};
            mt19937 rng_engine {seed};
            uniform_int_distribution<int> dist {low, high};
            return dist(rng_engine);
        };

        auto maze_factory = [](mazes::maze_types maze_type) {
            switch (maze_type) {
                case mazes::maze_types::BINARY_TREE: 
                    return std::make_unique<mazes::binary_tree>();
                // case mazes::maze_factory_types::SIDEWINDER:
                //     return std::make_unique<mazes::sidewinder>();
            }
        };
        
        craft maze_builder {"craft-sdl3", mazes::maze_types::BINARY_TREE};
        mazes::grid init_grid {10, 10};
        bool success = maze_builder.run(init_grid, get_int, args.is_interactive());
        if (success) {
            cout << init_grid << endl;
        } else {
            cerr << "ERROR: " << args.get_algo() << " failed!!" << endl;
        }
    
    } catch (std::exception& ex) {
        cerr << "ERROR: " << ex.what() << endl; 
    }

    return EXIT_SUCCESS;
}
