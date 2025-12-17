#include "config_mapper.h"

#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/string_utils.h>

#include <iostream>
#include <stdexcept>

bool config_mapper::map_args_to_config(std::vector<std::string> const& args, mazes::configurator& config) {

    auto set_config = [&config](const std::string& key, const std::string& value) {

        using namespace mazes;

        if (key == args::ROW_WORD_STR) {

            config.rows(std::stoi(value));
        } else if (key == args::COLUMN_WORD_STR) {

            config.columns(std::stoi(value));
        } else if (key == args::LEVEL_WORD_STR) {

            config.levels(std::stoi(value));
        } else if (key == args::ALGO_ID_WORD_STR) {

            config.algo_id(to_algo_from_sv(value));
        } else if (key == args::SEED_WORD_STR) {

            config.seed(std::stoi(value));
        } else if (key == args::BLOCK_ID_WORD_STR) {

            config.block_id(std::stoi(value));
        } else if (key == args::DISTANCES_WORD_STR) {
            // If distances key is present, enable distances
            // The value could be "true" for flag form, or "[start:end]" for range form
            if (value == args::TRUE_VALUE || !value.empty()) {

                config.distances(true);
            } else {
                config.distances(false);
            }
        } else if (key == args::DISTANCES_START_STR) {

            config.distances_start(std::stoi(value));
        } else if (key == args::DISTANCES_END_STR) {

            config.distances_end(std::stoi(value));
        } else if (key == args::OUTPUT_ID_WORD_STR) {

            if (value.empty()) {

                throw std::runtime_error("Output file name cannot be empty.");
            }
            
            // Store the full filename
            config.output_format_filename(value);
            
            auto extension = string_utils::get_file_extension(value);

            try {
                // Don't add the dot - to_output_format_from_sv expects extensions without dots
                config.output_format_id(to_output_format_from_sv(extension));
            } catch (const std::invalid_argument&) {
                // If extension isn't recognized, default to plain text
                config.output_format_id(output_format::STDOUT);
            }
        } else if (key == args::OUTPUT_FILENAME_WORD_STR) {

            config.output_format_filename(value);
        } else if (key == args::HELP_WORD_STR) {

            config.help(true);
        } else if (key == args::VERSION_WORD_STR) {

            config.version(true);
        }
        else {

            throw std::runtime_error("Unknown configuration option: " + key);
        }
    };

    try {

        mazes::args my_args;

        if (!my_args.parse(args)) {

            throw std::runtime_error("Failed to parse command line arguments.");
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

        throw std::runtime_error(std::string("config_mapper::map_args_to_config - ") + ex.what());
    }

    return true;
}
