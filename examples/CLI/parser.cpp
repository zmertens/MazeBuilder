#include "parser.h"

#include <MazeBuilder/args.h>

#include <cstring>
#include <iostream>
#include <stdexcept>

bool parser::parse(std::vector<std::string> const& args, mazes::configurator& config) const {
    
    using namespace std;

    auto set_config = [&config](const string& key, const string& value) {

        using namespace mazes;

        if (key == args::ROW_WORD_STR) {

            config.rows(stoi(value));
        } else if (key == args::COLUMN_WORD_STR) {

            config.columns(stoi(value));
        } else if (key == args::LEVEL_WORD_STR) {

            config.levels(stoi(value));
        } else if (key == args::ALGO_ID_WORD_STR) {

            config.algo_id(to_algo_from_sv(value));
        } else if (key == args::SEED_WORD_STR) {

            config.seed(stoi(value));
        } else if (key == args::BLOCK_ID_WORD_STR) {

            config.block_id(stoi(value));
        } else if (key == args::DISTANCES_WORD_STR) {
            // If distances key is present, enable distances
            // The value could be "true" for flag form, or "[start:end]" for range form
            if (value == args::TRUE_VALUE) {

                config.distances(true);
            } else if (!value.empty()) {

                // Any non-empty value (like "[0:5]") should enable distances
                config.distances(true);
            } else {
                config.distances(false);
            }
        } else if (key == args::DISTANCES_START_STR) {

            config.distances_start(stoi(value));
        } else if (key == args::DISTANCES_END_STR) {

            config.distances_end(stoi(value));
        } else if (key == args::OUTPUT_ID_WORD_STR) {

            if (value.empty()) {

                throw runtime_error("Output file name cannot be empty.");
            }

            // Detect if the value looks like a filename or format
            // If it contains a file extension or path separator, treat as filename
            // Otherwise, treat as format
            if (value.find('.') != string::npos || value.find('/') != string::npos || 
                value.find('\\') != string::npos || value == "stdout") {
                // This looks like a filename or special output (stdout)
                config.output_format_filename(value);
                
                // Try to infer format from filename extension
                if (value == "stdout") {
                    config.output_format_id(output_format::STDOUT);
                } else {
                    size_t dot_pos = value.find_last_of('.');
                    if (dot_pos != string::npos) {
                        string extension = value.substr(dot_pos + 1);
                        try {
                            config.output_format_id(to_output_format_from_sv(extension));
                        } catch (const invalid_argument&) {
                            // If extension isn't recognized, default to plain text
                            config.output_format_id(output_format::PLAIN_TEXT);
                        }
                    } else {
                        // No extension, default to plain text
                        config.output_format_id(output_format::PLAIN_TEXT);
                    }
                }
            } else {
                // This looks like a format specifier
                config.output_format_id(to_output_format_from_sv(value));
            }
        } else if (key == args::OUTPUT_FILENAME_WORD_STR) {

            config.output_format_filename(value);
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
            mazes::args::DISTANCES_START_STR,
            mazes::args::DISTANCES_END_STR,
            mazes::args::OUTPUT_ID_WORD_STR,
            mazes::args::OUTPUT_FILENAME_WORD_STR
        };
        
        // Process only the expected word keys to avoid processing duplicate entries
        for (const auto& key : word_keys) {

            if (auto value_opt = my_args.get(key);  value_opt.has_value()) {

                set_config(key, value_opt.value());

#if defined(MAZE_DEBUG)

                std::cerr << "Debug: Found key='" << key << "' value='" << value_opt.value() << "'" << std::endl;
#endif
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
