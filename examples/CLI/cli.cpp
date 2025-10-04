#include "cli.h"

#include <MazeBuilder/args.h>
#include <MazeBuilder/base64_helper.h>
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

// Use functions to avoid static initialization order mismatches
static std::string get_cli_version_str() {

    return "mazebuilder v" + mazes::buildinfo::Version + " (" + mazes::buildinfo::CommitSHA + ")";
}

static std::string get_cli_title_str() {

    return "mazebuilder v" + get_cli_version_str();
}

static std::string get_cli_help_str() {

    return get_cli_title_str() + "\n\n" + 
        "Generates mazes and converts to various formats\n\n" 
        "Example: ./cli -r 10 -c 10 -a binary_tree > maze.txt\n\n" 
        "Example: ./cli --rows=10 --columns=10 --algo=dfs -o maze.obj\n\n" 
        "Note: Commands are case-sensitive!\n\n"
        "\t-a, --algo         algorithm to generate maze links\n"
        "\t                     [binary_tree, dfs, sidewinder]\n" 
        "\t-c, --columns      columns\n" 
        "\t-d, --distances    show distances with optional [start, steps] inclusive\n"
        "\t                     example: '-d [0:10]'\n" 
        "\t-h, --help         display this help message\n" 
        "\t-j, --json         run with arguments in JSON format\n"
        "\t-s, --seed         seed for the number generator\n" 
        "\t-r, --rows         rows\n" 
        "\t-o, --output       output format\n"
        "\t                     [txt, json, obj, stdout]\n" 
        "\t-v, --version      display program version\n";
}

std::string cli::debug_str = "";

std::string cli::help_str = get_cli_help_str();

std::string cli::title_str = get_cli_title_str();

std::string cli::version_str = get_cli_version_str();

std::string cli::convert(std::vector<std::string> const& args_vec) const noexcept {

    using namespace std;

#if defined(MAZE_DEBUG)

    debug_str = version_str + " - DEBUG";
#endif

    if (args_vec.empty()) {

        return help_str;
    }

    try {

        if (auto need_help = find_if(args_vec.cbegin(), args_vec.cend(), [](const std::string& arg) {

                return arg == mazes::args::HELP_FLAG_STR || arg == mazes::args::HELP_OPTION_STR || arg == mazes::args::HELP_WORD_STR;
            }); need_help != args_vec.cend()) {

            return help_str;
        } else if (auto need_version = find_if(args_vec.cbegin(), args_vec.cend(), [](const std::string& arg) {

                return arg == mazes::args::VERSION_FLAG_STR || arg == mazes::args::VERSION_OPTION_STR || arg == mazes::args::VERSION_WORD_STR;
            }); need_version != args_vec.cend()) {

#if defined(MAZE_DEBUG)

            return debug_str;
#else

            return version_str;
#endif
        }

        parser my_parser;

        mazes::configurator temp_config;

        if (!my_parser.parse(cref(args_vec), ref(temp_config))) {

            throw std::runtime_error("Failed to parse command line arguments.");
        }

        // Store the configuration for later access
        m_config = make_shared<mazes::configurator>(temp_config);

        mazes::grid_factory factory;

        factory.register_creator(title_str, [](const mazes::configurator& config) -> std::unique_ptr<mazes::grid_interface> {

            return std::make_unique<mazes::distance_grid>(config.rows(), config.columns(), config.levels());
        });

        if (auto product = factory.create(title_str, *m_config.get()); product.has_value()) {

            mazes::randomizer rng;

            apply(product.value(), rng, m_config->algo_id(), *m_config.get());

            mazes::stringify maze_stringify;

            // Check if we need to generate Wavefront OBJ output
            if (m_config->output_format_id() == mazes::output_format::WAVEFRONT_OBJECT_FILE) {

                // Execute the stringify algorithm on the grid product
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

                if (!obj_helper.run(product.value().get(), std::ref(rng))) {

                    throw std::runtime_error("Failed to generate Wavefront OBJ data.");
                }
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

std::string cli::convert_as_base64(std::vector<std::string> const& args_vec) const noexcept {

    return mazes::base64_helper::encode(convert(std::cref(args_vec)));
}

std::string cli::help() const noexcept {

    return help_str;
}

std::string cli::version() const noexcept {

    return version_str;
}

/// @brief Get the configuration from the last convert call
/// @return The configuration object, or nullptr if no valid configuration exists
std::shared_ptr<mazes::configurator> cli::get_config() const noexcept {

    return m_config;
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

            throw std::invalid_argument("Unsupported algorithm: " + std::string{mazes::to_sv_from_algo(a)});
        } // switch

        if (!success) {

            throw std::runtime_error("Failed to run algorithm: " + std::string{mazes::to_sv_from_algo(a)});
        }

        // Calculate distances after maze generation if requested
        if (config.distances()) {

            // Try to cast to distance_grid to call calculate_distances
            if (auto distance_grid_ptr = dynamic_cast<mazes::distance_grid*>(g.get())) {

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

                distance_grid_ptr->calculate_distances(start_idx, end_idx);

#if defined(MAZE_DEBUG)

                std::cerr << "Debug: Calling calculate_distances with start="
                    << start_idx << ", end=" << end_idx << std::endl;

                if (auto distances = distance_grid_ptr->get_distances()) {

                    std::cerr << "Debug: Distances object created successfully" << std::endl;
                } else {

                    std::cerr << "Debug: Failed to create distances object" << std::endl;
                }
#endif
            } else {

#if defined(MAZE_DEBUG)

                std::cerr << "Debug: Failed to calculate distances" << std::endl;
#endif
            }
        }

    } catch (const std::exception& ex) {

#if defined(MAZE_DEBUG)

        std::cerr << "Algorithm Error: " << ex.what() << std::endl;
#endif
    } // catch
} // apply
