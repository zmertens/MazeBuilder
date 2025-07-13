#include "parser.h"

#include <MazeBuilder/args.h>

#include <cstring>
#include <iostream>
#include <stdexcept>

bool parser::parse(std::vector<std::string> const& args, mazes::configurator& config) const {
    
    using namespace std;

    auto set_config = [&config](const string& key, const string& value) {

        using namespace mazes;

        if (key == args::HELP_WORD_STR) {

            config.help(value);
        } else if (key == args::VERSION_WORD_STR) {

            config.version(value);
        } else if (key == args::ROW_WORD_STR) {

            config.rows(stoi(value));
        } else if (key == args::COLUMN_WORD_STR) {

            config.columns(stoi(value));
        } else if (key == args::LEVEL_WORD_STR) {

            config.levels(stoi(value));
        } else if (key == args::ALGO_ID_WORD_STR) {

            config.algo_id(to_algo_from_string(value));
        } else if (key == args::SEED_WORD_STR) {

            config.seed(stoi(value));
        } else if (key == args::BLOCK_ID_WORD_STR) {

            config.block_id(stoi(value));
        } else if (key == args::DISTANCES_WORD_STR) {

            config.distances(value == args::TRUE_VALUE);
        } else if (key == args::OUTPUT_ID_WORD_STR) {

            if (value.empty()) {

                throw runtime_error("Output file name cannot be empty.");
            }

            config.output_id(to_output_from_string(value));
        }
        else {

            throw runtime_error("Unknown configuration option: " + key);
        }
    };

    try {

        mazes::args my_args;

        if (!my_args.parse(args)) {

            throw runtime_error("Failed to parse command line arguments.");
        }

        // Only process the "word" form of each argument to avoid duplicate processing
        // and unrecognized key errors
        static const std::vector<std::string> word_keys = {
            mazes::args::HELP_WORD_STR,
            mazes::args::VERSION_WORD_STR,
            mazes::args::ROW_WORD_STR,
            mazes::args::COLUMN_WORD_STR,
            mazes::args::LEVEL_WORD_STR,
            mazes::args::ALGO_ID_WORD_STR,
            mazes::args::SEED_WORD_STR,
            mazes::args::BLOCK_ID_WORD_STR,
            mazes::args::DISTANCES_WORD_STR,
            mazes::args::OUTPUT_ID_WORD_STR
        };
        
        // Process only the expected word keys to avoid processing duplicate entries
        for (const auto& key : word_keys) {
            auto value_opt = my_args.get(key);
            if (value_opt.has_value()) {
                set_config(key, value_opt.value());
            }
        }

    } catch (const std::exception& ex) {

#if defined(MAZE_DEBUG)

        std::cerr << "Parser Error: " << ex.what() << std::endl;
#endif

        return false;
    }

    return true;
}
