#include <random>
#include <memory>
#include <exception>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <iomanip>

#include <MazeBuilder/maze_builder.h>

#include "cli.h"
#include "batch_processor.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten/bind.h>

// This is necessary due to emscripten's handling of templates
std::shared_ptr<cli> get() {
    return mazes::singleton_base<cli>::instance();
}

EMSCRIPTEN_BINDINGS(cli_module) {
    emscripten::function("get", &get, emscripten::allow_raw_pointers());
    emscripten::class_<cli>("cli")
        .smart_ptr<std::shared_ptr<cli>>("std::shared_ptr<cli>")
        .function("stringify_from_dimens", &cli::stringify_from_dimens);
}

#endif // EMSCRIPTEN_BINDINGS

static std::string MAZE_BUILDER_VERSION = "maze_builder\t" + mazes::VERSION;

int main(int argc, char* argv[]) {

    using namespace std;

#if defined(MAZE_DEBUG)
    MAZE_BUILDER_VERSION += " - DEBUG";
#endif

    static const std::string MAZE_BUILDER_HELP = MAZE_BUILDER_VERSION + "\n" + \
        "Usages: app.exe [OPTION(S)]... [OUTPUT]\n" \
        "Generates mazes and exports to different formats\n" \
        "Options: case-sensitive, long options must use '=' combination\n" \
        "Example: app.exe -r 10 -c 10 -a binary_tree > out_maze.txt\n" \
        "Example: app.exe --rows=10 --columns=10 --algo=dfs -o out_maze.txt\n" \
        "\t-a, --algo         [dfs] [sidewinder], [binary_tree]\n" \
        "\t-c, --columns      columns\n" \
        "\t-d, --distances    show distances using base36 numbers\n" \
        "\t-e, --encode       encode maze to base64 string\n" \
        "\t-h, --help         display this help message\n" \
        "\t-j, --json         run with arguments in JSON format\n" \
        "\t                   supports both single objects and arrays of objects\n" \
        "\t-s, --seed         seed for the mt19937 generator\n" \
        "\t-r, --rows         rows\n" \
        "\t-o, --output       [txt|text] [json] [jpg|jpeg] [png] [obj|object] [stdout]\n" \
        "\t-v, --version      display program version\n";

#if !defined(__EMSCRIPTEN__)

    // Copy command arguments and skip the program name
    vector<string> args_vec{ argv + 1, argv + argc };

    mazes::args maze_args{ };
    if (!maze_args.parse(args_vec)) {
        cerr << "Invalid arguments, parsing failed." << endl;
        return EXIT_FAILURE;
    }

    if (maze_args.get("-h").has_value() || maze_args.get("--help").has_value()) {
        cout << MAZE_BUILDER_HELP << endl;
        return EXIT_SUCCESS;
    }

    if (maze_args.get("-v").has_value() || maze_args.get("--version").has_value()) {
        cout << MAZE_BUILDER_VERSION << endl;
        return EXIT_SUCCESS;
    }

    try {
        // Get output file path
        string output_file_str = "stdout";
        if (maze_args.get("-o").has_value()) {
            output_file_str = maze_args.get("-o").value();
        } else if (maze_args.get("--output").has_value()) {
            output_file_str = maze_args.get("--output").value();
        }
        
        // Check if we're dealing with batch processing
        // if (maze_args.has_array()) {
        //     // Process all configurations in the array
        //     mazes::batch_processor processor;
        //     bool success = processor.process_batch(maze_args.get_array(), output_file_str);
            
        //     if (!success) {
        //         cerr << "Batch processing failed." << endl;
        //         return EXIT_FAILURE;
        //     }        } else {
        //     // Process a single maze with the configuration
        //     mazes::batch_processor processor;
        //     bool success = processor.process_single(maze_args.get(), output_file_str);
            
        //     if (!success) {
        //         cerr << "Maze processing failed." << endl;
        //         return EXIT_FAILURE;
        //     }
        // }
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

#endif // EMSCRIPTEN

    return EXIT_SUCCESS;
} // main
