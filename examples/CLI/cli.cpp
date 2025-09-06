#include "cli.h"

#include <MazeBuilder/args.h>
#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/stringify.h>
#include <MazeBuilder/objectify.h>
#include <MazeBuilder/wavefront_object_helper.h>

#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>

#include "parser.h"

const std::string cli::DEBUG_STR = "DEBUG";

// Use functions to avoid static initialization order mismatches
static std::string get_cli_version_str() {

    return mazes::build_info::Version + " (" + mazes::build_info::CommitSHA + ")";
}

static std::string get_cli_title_str() {
    return "mazebuildercli v" + get_cli_version_str();
}

static std::string get_cli_help_str() {
    return get_cli_title_str() + "\n" + 
        "Description: Generates mazes and outputs into string formats\n" 
        "Example: app.exe -r 10 -c 10 -a binary_tree > out_maze.txt\n" 
        "Example: app.exe --rows=10 --columns=10 --algo=dfs -o out_maze.txt\n" 
        "Note: Commands are case-sensitive!"
        "\t-a, --algo         binary_tree, dfs, sidewinder\n" 
        "\t-c, --columns      columns\n" 
        "\t-d, --distances    show distances in sliced array format; ex: '[0:1]'\n" 
        "\t-e, --encode       encode maze to base64 string\n" 
        "\t-h, --help         display this help message\n" 
        "\t-j, --json         run with arguments in JSON format\n" 
        "\t                   supports both single objects and arrays of objects\n" 
        "\t-s, --seed         seed for the mt19937 generator\n" 
        "\t-r, --rows         rows\n" 
        "\t-o, --output       txt, text, json, obj, object, stdout\n" 
        "\t-v, --version      display program version\n";
}

// Lazy initialization using function statics
std::string cli::CLI_VERSION_STR = get_cli_version_str();
std::string cli::CLI_TITLE_STR = get_cli_title_str();
std::string cli::CLI_HELP_STR = get_cli_help_str();

std::string cli::convert(std::vector<std::string> const& args_vec) const noexcept {

    using namespace std;

#if defined(MAZE_DEBUG)
    // Note: Don't modify static variables at runtime in a const noexcept function
    std::string debug_version = CLI_VERSION_STR + " - " + DEBUG_STR;
#endif

    try {

        if (auto need_help = find_if(args_vec.begin(), args_vec.end(), [](const std::string& arg) {
                return arg == mazes::args::HELP_FLAG_STR || arg == mazes::args::HELP_OPTION_STR || arg == mazes::args::HELP_WORD_STR;
            }); need_help != args_vec.end()) {

            return CLI_HELP_STR;
        } else if (auto need_version = find_if(args_vec.begin(), args_vec.end(), [](const std::string& arg) {
                return arg == mazes::args::VERSION_FLAG_STR || arg == mazes::args::VERSION_OPTION_STR || arg == mazes::args::VERSION_WORD_STR;
            }); need_version != args_vec.end()) {

#if defined(MAZE_DEBUG)
            return debug_version;
#else
            return CLI_VERSION_STR;
#endif
        }

        parser my_parser;

        mazes::configurator config;

        if (!my_parser.parse(cref(args_vec), ref(config))) {

            throw std::runtime_error("Failed to parse command line arguments.");
        }

        // Store the configuration for later access
        m_last_config = std::make_shared<mazes::configurator>(config);

        mazes::grid_factory factory;

        factory.register_creator("temp", [](const mazes::configurator& config) -> std::unique_ptr<mazes::grid_interface> {

            return std::make_unique<mazes::distance_grid>(config.rows(), config.columns(), config.levels());
        });

        if (auto product = factory.create("temp", config); product.has_value()) {

            mazes::randomizer rng;

            apply(product.value(), rng, config.algo_id(), config);

            // Check if we need to generate Wavefront OBJ output
            if (config.output_format_id() == mazes::output_format::WAVEFRONT_OBJECT_FILE) {
                // First, get the string representation for parsing
                mazes::stringify maze_stringify;
                if (!maze_stringify.run(product.value().get(), rng)) {

                    throw std::runtime_error("Failed to stringify maze for objectify processing.");
                }
                
                // Generate 3D object data
                mazes::objectify maze_objectify;
                if (!maze_objectify.run(product.value().get(), rng)) {

                    throw std::runtime_error("Failed to generate 3D object data.");
                }

                // Convert to Wavefront OBJ format
                mazes::wavefront_object_helper obj_helper;
                auto vertices = product.value()->operations().get_vertices();
                auto faces = product.value()->operations().get_faces();

                std::string obj_str = obj_helper.to_wavefront_object_str(vertices, faces);
                product.value()->operations().set_str(obj_str);
            } else {

                // Use the regular stringify process
                mazes::stringify maze_stringify;
                if (!maze_stringify.run(product.value().get(), rng)) {

                    throw std::runtime_error("Failed to stringify maze.");
                }
            }

            return product.value()->operations().get_str();
        }

    } catch (const std::exception& ex) {

#if defined(MAZE_DEBUG)

        std::cerr << "CLI Error: " << ex.what() << std::endl;
#endif
    }

    return "";
} // convert

std::string cli::help() const noexcept {

    return CLI_HELP_STR;
}

std::string cli::version() const noexcept {

    return CLI_VERSION_STR;
}

/// @brief Get the configuration from the last convert call
/// @return The configuration object, or nullptr if no valid configuration exists
std::shared_ptr<mazes::configurator> cli::get_config() const noexcept {
    return m_last_config;
}

/// @brief Apply an algorithm to the grid
/// @param g 
/// @param rng
/// @param a 
/// @param config
void cli::apply(std::unique_ptr<mazes::grid_interface> const& g, mazes::randomizer& rng, mazes::algo a, const mazes::configurator& config) const noexcept {

    using namespace std;

    try {

        bool success = false;

        switch (a) {

        case mazes::algo::BINARY_TREE: {

            static mazes::binary_tree bt;

            success = bt.run(g.get(), ref(rng));

            break;
        }
        case mazes::algo::SIDEWINDER: {

            static mazes::sidewinder sw;

            success = sw.run(g.get(), ref(rng));

            break;
        }
        case mazes::algo::DFS: {

            static mazes::dfs d;

            success = d.run(g.get(), ref(rng));

            break;
        }
        default:

            throw std::invalid_argument("Unsupported algorithm: " + mazes::to_string_from_algo(a));
        } // switch

        if (!success) {

            throw std::runtime_error("Failed to run algorithm: " + mazes::to_string_from_algo(a));
        }

        // Calculate distances after maze generation if requested
        if (config.distances()) {
#if defined(MAZE_DEBUG)
            std::cerr << "Debug: Distance calculation requested. Start: " << config.distances_start() 
                     << ", End: " << config.distances_end() << std::endl;
#endif
            // Try to cast to distance_grid to call calculate_distances
            if (auto distance_grid_ptr = dynamic_cast<mazes::distance_grid*>(g.get())) {
#if defined(MAZE_DEBUG)
                std::cerr << "Debug: Successfully cast to distance_grid" << std::endl;
#endif
                int start_idx = config.distances_start();
                int end_idx = config.distances_end();
                
                // If end index is -1 (default), use the last cell
                if (end_idx == -1) {
                    end_idx = (config.rows() * config.columns()) - 1;
                }
                
                // Ensure indices are within valid range
                int max_cell_index = (config.rows() * config.columns()) - 1;
                start_idx = std::max(0, std::min(start_idx, max_cell_index));
                end_idx = std::max(0, std::min(end_idx, max_cell_index));
                
#if defined(MAZE_DEBUG)
                std::cerr << "Debug: Calling calculate_distances with start=" << start_idx 
                         << ", end=" << end_idx << std::endl;
#endif
                distance_grid_ptr->calculate_distances(start_idx, end_idx);
#if defined(MAZE_DEBUG)
                auto distances = distance_grid_ptr->get_distances();
                if (distances) {
                    std::cerr << "Debug: Distances object created successfully" << std::endl;
                } else {
                    std::cerr << "Debug: Failed to create distances object" << std::endl;
                }
#endif
            } else {
#if defined(MAZE_DEBUG)
                std::cerr << "Debug: Failed to cast to distance_grid" << std::endl;
#endif
            }
        }

    } catch (const std::exception& ex) {

#if defined(MAZE_DEBUG)

        std::cerr << "Algorithm Error: " << ex.what() << std::endl;
#endif
    } // catch
} // apply
