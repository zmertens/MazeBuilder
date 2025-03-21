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
          -a, --algo         dfs, sidewinder, binary_tree [default]
          -s, --seed         seed for the mt19937 generator [default=0]
          -r, --rows         maze rows [default=100]
          -l, --levels       maze height [default=10]
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

    // Copy command arguments and skip the program name
    vector<string> args_vec{ argv + 1, argv + argc };

    mazes::args maze_args{ };
    if (!maze_args.parse(args_vec)) {
        cerr << "Invalid arguments" << endl;
        return EXIT_FAILURE;
    }

    if (maze_args.get("-h").has_value() || maze_args.get("--help").has_value()) {
        cout << MAZE_BUILDER_HELP << endl;
        return EXIT_SUCCESS;
    }

    if (maze_args.get("-v").has_value() || maze_args.get("--version").has_value()) {
        cout << maze_builder_version << endl;
        return EXIT_SUCCESS;
    }

    auto seed = 0;
    
    if (maze_args.get("-s").has_value()) {
        seed = atoi(maze_args.get("-s").value().c_str());
    } else if (maze_args.get("--seed").has_value()) {
        seed = atoi(maze_args.get("--seed").value().c_str());
    }

    std::mt19937 rng_engine{ static_cast<unsigned long>(seed)};
    auto get_int = [&rng_engine](int low, int high) -> int {
        uniform_int_distribution<int> dist {low, high};
        return dist(rng_engine);
    };

    string algo_str = "";
    if (maze_args.get("-a").has_value()) {
        algo_str = maze_args.get("-a").value();
    } else if (maze_args.get("--algo").has_value()) {
        algo_str = maze_args.get("--algo").value();
    }

    // Run the command-line program

    try {
        
        bool success = false;

        mazes::algo my_maze_type = mazes::to_algo_from_string(cref(algo_str));

        auto rows = 0, columns = 0, levels = 1;
        if (maze_args.get("-c").has_value()) {
            columns = atoi(maze_args.get("-c").value().c_str());
        } else if (maze_args.get("--columns").has_value()) {
            columns = atoi(maze_args.get("--columns").value().c_str());
        }

        if (maze_args.get("-r").has_value()) {
            rows = atoi(maze_args.get("-r").value().c_str());
        } else if (maze_args.get("--rows").has_value()) {
            rows = atoi(maze_args.get("--rows").value().c_str());
        }

        static constexpr auto BLOCK_ID = -1;

        auto next_maze_ptr = mazes::factory::create(
            mazes::configurator().columns(columns).rows(rows).levels(levels)
            .distances(false).seed(seed)._algo(my_maze_type)
            .block_id(BLOCK_ID));

        if (next_maze_ptr.has_value()) {
            auto maze_s = mazes::stringz::stringify(cref(next_maze_ptr.value()));
            cout << maze_s << endl;
            success = true;
        }

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
            //auto elapsedms = progress.elapsed_ms();
            //progress.reset();
#if defined(MAZE_DEBUG)
            //std::cout << "Writing to file: " << maze_args.output << " complete!!" << std::endl;
            //std::cout << "Progress: " << elapsedms << " seconds" << std::endl;
#endif
        }
        else {

        }
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
} // main
