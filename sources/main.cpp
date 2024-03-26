#include <random>
#include <memory>
#include <exception>
#include <iostream>
#include <string_view>
#include <sstream>
#include <future>

#include "craft.h"
#include "grid.h"
#include "binary_tree.h"
#include "sidewinder.h"
#include "args_builder.h"
#include "maze_types_enum.h"
#include "writer.h"

// Struggling with CMake build config and so I added this for Release builds
//#if defined(DEBUGGING)
//#undef DEBUGGING
//#endif

int main(int argc, char* argv[]) {

    static constexpr auto MAZE_BUILDER_VERSION = "maze_builder=[2.3.0]";

    static constexpr auto HELP_MSG = R"help(
        Usages: maze_builder [OPTION]... [OUT_FILE]
        Generates mazes in ASCII-format or Wavefront object format
        Example: maze_builder -w 10 -l 10 -a binary_tree > out_maze.txt
        Options specify how to generate the maze and file output:
          -a, --algorithm    binary_tree [default], sidewinder
          -s, --seed         seed for the random number generator [mt19937]
          -w, --width        maze width [default=100]
          -y, --height       maze height [default=10]
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
        auto&& args_map {args.build()};
        // this needs to get called after args.build() because of internal parsing
        auto state_of_args{ args.get_state() };
        
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

        auto _grid {std::make_unique<mazes::grid>(args.get_width(), args.get_length(), args.get_height())};

        auto maze_factory = [&_grid, &get_int](mazes::maze_types maze_type) {
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
                        return sw.run(std::ref(_grid), get_int);
                    });
                }
            }
        };
        mazes::writer my_writer;
        auto write_func = [&my_writer, &args](auto data)->bool {
            return my_writer.write(args.get_output(), data);
        };
        std::packaged_task<bool(const std::string& data)> task_writes (write_func);

        mazes::maze_types maze_algo = get_maze_type_from_algo(args.get_algorithm());
        bool success = false;
        if (args.is_interactive()) {
            // string views don't own the data, they have less copying overhead
            std::string_view sv {"craft-sdl3"};
            std::string_view version_view{ MAZE_BUILDER_VERSION };
            craft maze_builder_3D {sv, version_view, maze_factory};
            success = maze_builder_3D.run(_grid, get_int, args.is_interactive());
        } else {
            success = maze_factory(maze_algo).get();
        }

        if (success && !args.is_interactive()) {
            auto&& fut_writer = task_writes.get_future();
            std::stringstream ss;
            ss << *_grid.get();
            task_writes(ss.str());
            if (fut_writer.get()) {
#if defined(DEBUGGING)
                std::cout << "Writing to file: " << args.get_output() << " complete!!" << std::endl;
#endif
            } else {
                std::cerr << "ERROR: Writing to file " << args.get_output() << std::endl;
            }
        } else if (!success){
            std::cerr << "ERROR: " << args.get_algorithm() << " failed!!" << std::endl;
        }
    
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
}
