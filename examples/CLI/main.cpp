#include <random>
#include <memory>
#include <exception>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>
#include <list>

#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/distances.h>
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
        Example: maze_builder.exe -r 10 -c 10 -a binary_tree > out_maze.txt
          -a, --algorithm    dfs, sidewinder, binary_tree [default]
          -s, --seed         seed for the mt19937 generator [default=0]
          -r, --rows         maze rows [default=100]
          -y, --height       maze height [default=10]
          -c, --columns      maze columns [default=100]
          -d, --distances    show distances in the maze
          -i, --interactive  No effect
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

    try {
        static constexpr auto CELL_SIZE = 10;
        bool success = false;
        // Run the command-line program
        mazes::maze_types my_maze_type = mazes::to_maze_type(maze_args.algorithm);
        static constexpr auto block_type = -1;
        mazes::maze_builder builder;
        auto my_maze = builder.rows(maze_args.rows)
            .columns(maze_args.columns)
            .height(maze_args.height)
            .maze_type(my_maze_type)
            .get_int(get_int)
            .rng(rng_engine)
            .block_type(block_type)
            .show_distances(maze_args.distances)
            .build();
        my_maze->start_progress();
        mazes::writer my_writer;
        mazes::output_types my_output_type = my_writer.get_output_type(maze_args.output);
        switch (my_output_type) {
        case mazes::output_types::WAVEFRONT_OBJ_FILE:
            success = my_writer.write(cref(maze_args.output), my_maze->to_wavefront_obj_str());
            break;
        case mazes::output_types::PNG:
            success = my_writer.write_png(cref(maze_args.output), 
            my_maze->to_pixels(CELL_SIZE), maze_args.rows * CELL_SIZE, maze_args.columns * CELL_SIZE);
            break;
        case mazes::output_types::PLAIN_TEXT: [[fallthrough]];
        case mazes::output_types::STDOUT: {
            string maze_str = my_maze->to_str();
            success = my_writer.write(cref(maze_args.output), cref(maze_str));
            break;
        }
        case mazes::output_types::UNKNOWN:
            success = false;
            break;
        }

        if (success) {
            my_maze->stop_progress();
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
