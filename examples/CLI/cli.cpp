#include "cli.h"

#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/stringify.h>

#include <functional>
#include <iostream>
#include <stdexcept>

#include "parser.h"

const std::string cli::DEBUG_STR = "DEBUG";

std::string cli::CLI_VERSION_STR = mazes::build_info::Version + " (" + mazes::build_info::CommitSHA + ") ";

std::string cli::CLI_HELP_STR = cli::CLI_TITLE_STR + "\n" + \
    "Description: Generates mazes and outputs into string formats\n" \
    "Example: app.exe -r 10 -c 10 -a binary_tree > out_maze.txt\n" \
    "Example: app.exe --rows=10 --columns=10 --algo=dfs -o out_maze.txt\n" \
    "\t-a, --algo         binary_tree, dfs, sidewinder\n" \
    "\t-c, --columns      columns\n" \
    "\t-d, --distances    show distances with start, end positions\n" \
    "\t                   ex: -d [0:10] or --distances=[1:]\n" \
    "\t-e, --encode       encode maze to base64 string\n" \
    "\t-h, --help         display this help message\n" \
    "\t-j, --json         run with arguments in JSON format\n" \
    "\t                   supports both single objects and arrays of objects\n" \
    "\t-s, --seed         seed for the mt19937 generator\n" \
    "\t-r, --rows         rows\n" \
    "\t-o, --output       [txt|text] [json] [jpg|jpeg] [png] [obj|object] [stdout]\n" \
    "\t-v, --version      display program version\n";

std::string cli::CLI_TITLE_STR = "mazebuildercli v" + cli::CLI_VERSION_STR;

std::string cli::convert(std::vector<std::string> const& args_vec) const noexcept {

    using namespace std;

#if defined(MAZE_DEBUG)

    CLI_VERSION_STR += "- " + DEBUG_STR;
#endif

    try {

        parser my_parser;

        mazes::configurator config;

        if (!my_parser.parse(cref(args_vec), ref(config))) {

            throw std::runtime_error("Failed to parse command line arguments.");
        }

        if (!config.help().empty()) {

            return CLI_HELP_STR;
        } else if (!config.version().empty()) {

            return CLI_VERSION_STR;
        }

        mazes::grid_factory factory;

        std::unique_ptr<mazes::grid_interface> product = factory.create(cref(config));

        mazes::randomizer rng;

        apply(cref(product), ref(rng), config.algo_id());

        mazes::stringify maze_stringify;

        if (!maze_stringify.run(cref(product), ref(rng))) {

            throw std::runtime_error("Failed to stringify maze.");
        }

        return product->operations().get_str();

    } catch (const std::exception& ex) {

#if defined(MAZE_DEBUG)

        std::cerr << "CLI Error: " << ex.what() << std::endl;
#endif

        return "";
    }
} // convert

/// @brief Apply an algorithm to the grid
/// @param g 
/// @param a 
void cli::apply(std::unique_ptr<mazes::grid_interface> const& g, mazes::randomizer& rng, mazes::algo a) const noexcept {

    using namespace std;

    try {

        bool success = false;

        switch (a) {

        case mazes::algo::BINARY_TREE: {

            static mazes::binary_tree bt;

            success = bt.run(cref(g), ref(rng));

            break;
        }
        case mazes::algo::SIDEWINDER: {

            static mazes::sidewinder sw;

            success = sw.run(cref(g), ref(rng));

            break;
        }
        case mazes::algo::DFS: {

            static mazes::dfs d;

            success = d.run(cref(g), ref(rng));

            break;
        }
        default:

            throw std::invalid_argument("Unsupported algorithm: " + mazes::to_string_from_algo(a));
        } // switch

        if (!success) {

            throw std::runtime_error("Failed to run algorithm: " + mazes::to_string_from_algo(a));
        }

    } catch (const std::exception& ex) {

#if defined(MAZE_DEBUG)

        std::cerr << "Algorithm Error: " << ex.what() << std::endl;
#endif
    } // catch
} // apply
