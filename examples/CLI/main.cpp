#include <random>
#include <memory>
#include <exception>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>
#include <list>

#include <MazeBuilder/distances.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/args_builder.h>
#include <MazeBuilder/output_types_enum.h>
#include <MazeBuilder/maze_factory.h>
#include <MazeBuilder/maze_builder.h>
#include <MazeBuilder/writer.h>

std::string maze_builder_version = "maze_builder=[5.1.5]";

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

int main(int argc, char* argv[]) {

    using namespace std;

#if defined(MAZE_DEBUG)
    maze_builder_version += " - DEBUG";
#endif

    vector<string> args_vec{ argv, argv + argc };

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
        // Run the command-line program
        mazes::maze_types my_maze_type = get_maze_type_from_algo(maze_args.algorithm);
        static constexpr auto block_type = -1;
        mazes::maze_builder my_maze{ maze_args.width, maze_args.length, maze_args.height };
        my_maze.start_progress();
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
        case mazes::output_types::STDOUT: {
            string maze_str = my_maze.to_str(my_maze_type, std::cref(get_int), std::cref(rng_engine), maze_args.distances);
            success = my_writer.write(cref(maze_args.output), cref(maze_str));
            break;
        }
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
            std::cerr << "ERROR: " << maze_args.algorithm << " failed!!" << std::endl;
            std::cerr << "ERROR: Writing to file: " << maze_args.output << std::endl;
        }
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
} // main
