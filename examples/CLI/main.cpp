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

static std::string MAZE_BUILDER_VERSION = "maze_builder\t" + mazes::VERSION;

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

int main(int argc, char* argv[]) {

    using namespace std;

#if defined(MAZE_DEBUG)
    MAZE_BUILDER_VERSION += " - DEBUG";
#endif

    static const std::string MAZE_BUILDER_HELP = MAZE_BUILDER_VERSION + "\n" + \
        "Description: Generates mazes in different string forms\n" \
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

        auto config = maze_args.get_configuration(0);
        if (config.has_value()) {
            std::cout << "   Configuration ready for batch processing:\n";
            
            // This is how batch_processor.extract_params() would access the config
            auto it = config->find(mazes::args::ROW_WORD_STR);
            if (it != config->end()) {
                std::cout << "   Found rows: " << it->second << "\n";
            }

            it = config->find(mazes::args::COLUMN_WORD_STR);
            if (it != config->end()) {
                std::cout << "   Found columns: " << it->second << "\n";
            }

            it = config->find(mazes::args::SEED_WORD_STR);
            if (it != config->end()) {
                std::cout << "   Found seed: " << it->second << "\n";
            }

            it = config->find(mazes::args::ALGO_WORD_STR);
            if (it != config->end()) {
                std::cout << "   Found algo: " << it->second << "\n";
            }
        }
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

#endif // EMSCRIPTEN

    return EXIT_SUCCESS;
} // main
