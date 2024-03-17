#include <random>
#include <memory>
#include <exception>
#include <cstdio>
#include <iostream>
#include <string_view>
#include <future>
#include <thread>

#include "craft.h"
#include "grid.h"
#include "binary_tree.h"
#include "sidewinder.h"
#include "args_builder.h"
#include "maze_types_enum.h"

int main(int argc, char* argv[]) {

    static constexpr auto MAZE_BUILDER_VERSION = "maze_builder=[2.0.1]";

    static constexpr auto HELP_MSG = R"help(
        Usages: maze_builder [OPTION]... [OUT_FILE]
        Run the builder to generate mazes from optional algorithms
        Example: maze_builder -w 10 -l 10 -a binary_tree > out_maze.txt\n
        Options specify how to generate the maze and file output:
          -a, --algorithm    binary_tree [default], sidewinder
          -s, --seed         seed for the random number generator [mt19937]
          -w, --width        maze width [default=100]
          -h, --height       maze height [default=10]
          -l, --length       maze length [default=100]
          -i, --interactive  run program in interactive mode with a GUI
          -o, --output       stdout [default], plain text [.txt] or Wavefront object format [.obj]
          -h, --help         display this help message
          -v, --version      display program version
    )help";

    std::vector<std::string> args_vec;
    for (int i = 0; i < argc; i++) {
        args_vec.emplace_back(argv[i]);
    }

    try {
        mazes::args_builder args (MAZE_BUILDER_VERSION, HELP_MSG, args_vec);
        mazes::args_state state_of_args {args.get_state()};
        auto&& args_map {args.build()};

#if defined(DEBUGGING)
        std::cout << args << std::endl;
#endif

        if (state_of_args == mazes::args_state::JUST_NEEDS_HELP) {
            std::cout << HELP_MSG << std::endl;
            return EXIT_SUCCESS;
        } else if (state_of_args == mazes::args_state::JUST_NEEDS_VERSION) {
            std::cout << MAZE_BUILDER_VERSION << std::endl;
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

        auto get_maze_type_from_algo = [](const std::string& algo)->mazes::maze_types {
            using namespace std;
            if (algo.compare("binary_tree") == 0) {
                return mazes::maze_types::BINARY_TREE;
            } else if (algo.compare("sidewinder") == 0) {
                return mazes::maze_types::SIDEWINDER;
            } else {
#if defined(DEBUGGING)
                cout << "INFO: Using default maze algorithm, \"binary_tree\"\n";
#endif
                return mazes::maze_types::BINARY_TREE;
            }
        };
        
        auto maze_factory = [&](mazes::maze_types maze_type, mazes::grid_ptr& _grid) {
            switch (maze_type) {
                case mazes::maze_types::BINARY_TREE: {
                    static mazes::binary_tree bt;
                    return std::async([&] {
                        return bt.run(std::ref(_grid), get_int);
                    });
                }
                case mazes::maze_types::SIDEWINDER: {
                    static mazes::sidewinder sw;
                    return std::async([&] {
                        return sw.run(std::ref(_grid), get_int, false);
                    });
                }
            }
        };

        std::string_view sv {"craft-sdl3"};
        mazes::maze_types maze_algo = get_maze_type_from_algo(args.get_algo());
        auto _grid {std::make_unique<mazes::grid>(25, 25)};
        craft maze_builder {sv, std::move(maze_factory(maze_algo, std::ref(_grid)))};
        auto&& success = maze_builder.run(_grid, get_int, args.is_interactive());
        // auto&& success = maze_factory(maze_algo, std::ref(_grid)).get();
        // _grid->print_grid_cells();
        if (success) {
            // Check grid size because terminal output can get smushed
            if (_grid->get_columns() < 10000 && _grid->get_rows() < 10000) {
                std::cout << *_grid.get() << std::endl;
            }
        } else {
            std::cerr << "ERROR: " << args.get_algo() << " failed!!" << std::endl;
        }
    
    } catch (std::exception& ex) {
        std::cerr << "ERROR: " << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
}
