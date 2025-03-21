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

    string output_str = "";
    if (maze_args.get("-o").has_value()) {
        output_str = maze_args.get("-o").value();
    } else if (maze_args.get("--output").has_value()) {
        output_str = maze_args.get("--output").value();
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

        using maze_ptr = optional<unique_ptr<mazes::maze>>;

        auto dur = mazes::progress<>::duration(mazes::factory::create,
            mazes::configurator().columns(columns).rows(rows).levels(levels)
            .distances(false).seed(seed)._algo(my_maze_type)
            .block_id(BLOCK_ID));

        maze_ptr next_maze_ptr = mazes::factory::create(
            mazes::configurator().columns(columns).rows(rows).levels(levels)
            .distances(false).seed(seed)._algo(my_maze_type)
            .block_id(BLOCK_ID));

        auto maze_s = mazes::stringz::stringify(cref(next_maze_ptr.value()));

        if (!next_maze_ptr.has_value()) {
            throw runtime_error("Failed to create maze");
        }

        mazes::writer my_writer;
        mazes::output my_output_type = mazes::to_output_from_string(output_str.substr(output_str.find_last_of(".") + 1));
        switch (my_output_type) {
        case mazes::output::WAVEFRONT_OBJECT_FILE: {
            vector<tuple<int, int, int, int>> vertices;
            vector<vector<uint32_t>> faces;
            mazes::wavefront_object_helper woh{};
            auto obj_str = woh.to_wavefront_object_str(cref(next_maze_ptr.value()), cref(vertices), cref(faces));
            success = my_writer.write(cref(output_str), cref(obj_str));
            break;
        }
        case mazes::output::PNG: {
            //success = my_writer.write_png(cref(maze_args.output), 
            //my_maze->to_pixels(CELL_SIZE), maze_args.rows * CELL_SIZE, maze_args.columns * CELL_SIZE);
            break;
        }
        case mazes::output::PLAIN_TEXT: {
            success = my_writer.write(cref(output_str), cref(maze_s));
            break;
        }
        case mazes::output::STDOUT: {
            cout << maze_s << endl;
            success = true;
            break;
        }
        default:
            success = false;
        }

        if (success) {
#if defined(MAZE_DEBUG)
            std::cout << "Writing to file: " << output_str << " complete!!" << std::endl;
            std::cout << "Duration: " << std::chrono::duration<double, std::milli>(dur).count() << " milliseconds" << std::endl;
#endif
        }
        else {
            std::cerr << "Failed output formatting to: " << output_str << std::endl;
        }
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl; 
    }

    return EXIT_SUCCESS;
} // main
