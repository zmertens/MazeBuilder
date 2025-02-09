#include <random>
#include <memory>
#include <exception>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>
#include <list>

#include <MazeBuilder/maze_builder.h>

std::string maze_builder_version = "maze_builder\tversion\t" + mazes::VERSION;

static constexpr auto MAZE_BUILDER_HELP = R"help(
        Usages: maze_builder.exe [OPTION(S)]... [OUTPUT]
        Generates mazes and exports to different formats
        Example: maze_builder.exe -r 10 -c 10 -a binary_tree > out_maze.txt
          -a, --algorithm    dfs, sidewinder, binary_tree [default]
          -s, --seed         seed for the mt19937 generator [default=0]
          -r, --rows         maze rows [default=100]
          -y, --height       maze height [default=10]
          -c, --columns      maze columns [default=100]
          -d, --distances    show distances in the maze
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

    mazes::args maze_args{ };
    if (!maze_args.parse(args_vec)) {
        cerr << "ERROR: Invalid arguments!!" << endl;
        return EXIT_FAILURE;
    }
    for (const auto& arg : args_vec) {
        if (arg.compare("-h") == 0 || arg.compare("--help") == 0) {
            std::cout << MAZE_BUILDER_HELP << std::endl;
            return EXIT_SUCCESS;
        }
        if (arg.compare("-v") == 0 || arg.compare("--version") == 0) {
            std::cout << maze_builder_version << std::endl;
            return EXIT_SUCCESS;
        }
    }
    
	// Set the version and help strings
	maze_args.version = maze_builder_version;
	maze_args.help = MAZE_BUILDER_HELP;

    std::mt19937 rng_engine{ static_cast<unsigned long>(maze_args.seed) };
    auto get_int = [&rng_engine](int low, int high) -> int {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng_engine);
    };

    try {
        static constexpr auto CELL_SIZE = 10;
        bool success = false;
        // Run the command-line program
        mazes::algos my_maze_type = mazes::to_maze_type(maze_args.algo);
        static constexpr auto block_type = -1;
        mazes::progress progress;
        mazes::builder builder;
        auto my_maze = builder.rows(maze_args.rows)
            .columns(maze_args.columns)
            .height(maze_args.height)
            .maze_type(my_maze_type)
            .block_type(block_type)
            .show_distances(maze_args.distances)
            .build();
        my_maze->init();
        mazes::computations::compute_geometry(my_maze);

        auto maze_s = mazes::tools::stringify(cref(my_maze->get_grid().value()));
        // cout << maze_s << endl;

        // mazes::writer my_writer;
        // mazes::outputs my_output_type = my_writer.get_output_type(maze_args.output);
        // switch (my_output_type) {
        // case mazes::outputs::WAVEFRONT_OBJ_FILE:
        //     success = my_writer.write(cref(maze_args.output), my_maze->to_wavefront_obj_str());
        //     break;
        // case mazes::outputs::PNG:
        //     success = my_writer.write_png(cref(maze_args.output), 
        //     my_maze->to_pixels(CELL_SIZE), maze_args.rows * CELL_SIZE, maze_args.columns * CELL_SIZE);
        //     break;
        // case mazes::outputs::PLAIN_TEXT: [[fallthrough]];
        // case mazes::outputs::STDOUT: {
        //     string maze_str = my_maze->to_str();
        //     success = my_writer.write(cref(maze_args.output), cref(maze_str));
        //     break;
        // }
        // case mazes::outputs::UNKNOWN:
        //     success = false;
        //     break;
        // }

        if (success) {
            auto elapsedms = progress.elapsed_ms();
            progress.reset();
#if defined(MAZE_DEBUG)
            std::cout << "INFO: Writing to file: " << maze_args.output << " complete!!" << std::endl;
            std::cout << "INFO: Progress: " << elapsedms << " seconds" << std::endl;
#endif
        }
        else {
            std::cerr << "ERROR: " << maze_args.algo << " failed!!" << std::endl;
            std::cerr << "ERROR: Writing to file: " << maze_args.output << std::endl;
        }
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
} // main
