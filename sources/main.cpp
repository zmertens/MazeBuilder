#include <random>
#include <memory>
#include <exception>
#include <iostream>
#include <sstream>
#include <future>
#include <thread>
#include <shared_mutex>
#include <algorithm>
#include <string>
#include <vector>
#include <list>

#include "distances.h"
#include "colored_grid.h"
#include "distance_grid.h"
#include "grid.h"
#include "args_builder.h"
#include "output_types_enum.h"
#include "maze_factory.h"
#include "maze_thread_safe.h"
#include "writer.h"
#include "craft.h"

std::string maze_builder_version = "maze_builder=[4.1.5]";

static constexpr auto MAZE_BUILDER_HELP = R"help(
        Usages: maze_builder.exe [OPTION(S)]... [OUTPUT]
        Generates mazes and exports to different formats
        Example: maze_builder.exe -w 10 -l 10 -a binary_tree > out_maze.txt
          -a, --algorithm    dfs, sidewinder, binary_tree [default]
          -s, --seed         seed for the mt19937 generator [default=0]
          -w, --width        maze width [default=100]
          -y, --height       maze height [default=10]
          -l, --length       maze length [default=100]
          -c, --cell_size    maze cell size [default=3]
          -d, --distances    show distances in the maze
          -i, --interactive  run program in interactive mode with a GUI
          -o, --output       [.txt], [.png], [.obj], [stdout[default]]
          -h, --help         display this help message
          -v, --version      display program version
    )help";

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

// bind a getter method from C++ so that it can be accessed in the frontend with JS
EMSCRIPTEN_BINDINGS(maze_builder_module) {
    emscripten::class_<craft>("craft")
        .smart_ptr<std::shared_ptr<craft>>("std::shared_ptr<craft>")
        .constructor<const std::string&, const std::string&, int, int>()
        .function("fullscreen", &craft::fullscreen)
        .function("mouse", &craft::mouse)
        .function("mazes", &craft::mazes)
        .class_function("get_instance", &craft::get_instance, emscripten::allow_raw_pointers());
}
#endif

int main(int argc, char* argv[]) {

    using namespace std;

#if defined(MAZE_DEBUG)
    maze_builder_version += " - DEBUG";
#endif

    vector<string> args_vec{ argv, argv + argc };

#if defined(__EMSCRIPTEN__)
    // Force the -i param when compiling to the web
    auto it_i = std::find(args_vec.begin(), args_vec.end(), "-i");
    auto it_interactive = std::find(args_vec.begin(), args_vec.end(), "--interactive");
    if (it_i == args_vec.end() && it_interactive == args_vec.end()) {
        args_vec.push_back("-i");
    }
#endif

    mazes::args_builder builder{ args_vec };
	mazes::args maze_args = builder.build();
    
    if (!maze_args.help.empty()) {
        std::cout << MAZE_BUILDER_HELP << std::endl;
        return EXIT_SUCCESS;
    } else if (!maze_args.version.empty()) {
        std::cout << maze_builder_version << std::endl;
        return EXIT_SUCCESS;
    }
	// Set the version and help strings
	builder.version(maze_builder_version);
	builder.help(MAZE_BUILDER_HELP);

    maze_args = builder.build();

    std::mt19937 rng_engine{ static_cast<unsigned long>(maze_args.seed) };
    auto get_int = [&rng_engine](int low, int high) -> int {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng_engine);
    };

    // Convert algorithm string into an enum type
    std::list<std::string> algos = { "binary_tree", "sidewinder", "dfs" };
    auto get_maze_type_from_algo = [](const std::string& algo)->mazes::maze_types {
        if (algo.compare("binary_tree") == 0) {
            return mazes::maze_types::BINARY_TREE;
        } else if (algo.compare("sidewinder") == 0) {
            return mazes::maze_types::SIDEWINDER;
        } else if (algo.compare("dfs") == 0) {
            return mazes::maze_types::DFS;
        } else {
            return mazes::maze_types::INVALID_ALGO;
        }
    };

    try {
        bool success = false;
        // Run the SDL app
        if (maze_args.interactive) {
            static constexpr int window_w = 800, window_h = 600;
            auto&& maze_builder_3D = craft::get_instance(cref(maze_args.version), cref(maze_args.help), window_w, window_h);
            success = maze_builder_3D->run(std::cref(algos), std::cref(get_maze_type_from_algo), std::cref(get_int), std::ref(rng_engine));
            if (!success) {
                std::cerr << "ERROR: Running SDL app failed." << std::endl;
            }
        } else {
            // Run the command-line program
            mazes::maze_types my_maze_type = get_maze_type_from_algo(maze_args.algorithm);
            static constexpr auto block_type = -1;
            mazes::maze_thread_safe my_maze{ maze_args.width, maze_args.length, maze_args.height };
            my_maze.start_progress();
			string maze_str = my_maze.to_str(my_maze_type, std::cref(get_int), std::cref(rng_engine), maze_args.distances);
            if (!maze_str.empty()) {
                mazes::writer my_writer;
				mazes::output_types my_output_type = my_writer.get_output_type(maze_args.output);
                switch (my_output_type) {
                case mazes::output_types::WAVEFRONT_OBJ_FILE:
                    my_maze.compute_geometry(my_maze_type, std::cref(get_int), std::cref(rng_engine), block_type);
                    success = my_writer.write(cref(maze_args.output), my_maze.to_wavefront_obj_str());
                    break;
                case mazes::output_types::PNG:
                    success = my_writer.write_png(cref(maze_args.output), 
                        my_maze.to_pixels(my_maze_type, std::cref(get_int), 
                            std::cref(rng_engine), maze_args.cell_size), 
                            maze_args.width * maze_args.cell_size, 
                            maze_args.length * maze_args.cell_size);
                    break;
                case mazes::output_types::PLAIN_TEXT: [[fallthrough]];
				case mazes::output_types::STDOUT:
					success = my_writer.write(cref(maze_args.output), cref(maze_str));
                    break;
                case mazes::output_types::UNKNOWN:
                    success = false;
                    break;
                }                
                if (success) {
                    my_maze.stop_progress();
#if defined(MAZE_DEBUG)
                    std::cout << "INFO: Writing to file: " << maze_args.output << " complete!!" << std::endl;
                    std::cout << "INFO: Progress: " << my_maze.get_progress_in_seconds() << " seconds" << std::endl;
#endif
                }
                else {
                    std::cerr << "ERROR: Writing to file: " << maze_args.output << std::endl;
                }
            }
            else {
                std::cerr << "ERROR: " << maze_args.algorithm << " failed!!" << std::endl;
            }
        }
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
}
