#include "cli.h"

#include <MazeBuilder/args.h>
#include <MazeBuilder/bytes.h>
#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/pixels.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/stringify.h>
#include <MazeBuilder/string_utils.h>
#include <MazeBuilder/objectify.h>
#include <MazeBuilder/wavefront_object_helper.h>

#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <ranges>
#include <stdexcept>

#include "config_mapper.h"

// Use functions to avoid static initialization order mismatches
static std::string get_cli_version_str()
{
    return mazes::string_utils::concat(mazes::string_utils::concat("mazebuilder v",
                                                                   mazes::buildinfo::Version),
                                       " - " + mazes::buildinfo::CommitSHA);
}

static std::string get_cli_title_str()
{
    return get_cli_version_str();
}

static std::string get_cli_help_str()
{
    return get_cli_title_str() + "\n\n" +
        "Generates mazes and converts to various formats\n\n"
        "Example: ./cli -r 14 -c 10 -a binary_tree > maze.txt\n\n"
        "Example: ./cli --rows=5 --columns=6 --algo=dfs -o maze.obj\n\n"
        "Note: Commands are case-sensitive!\n\n"
        "\t-a, --algo         algorithm to generate maze links\n"
        "\t                     [binary_tree, dfs, sidewinder]\n"
        "\t-c, --columns      columns [max: 100]\n"
        "\t-d, --distances    show distances with optional [start, steps] inclusive\n"
        "\t                     example: '-d [0:10]'\n"
        "\t-h, --help         display this help message\n"
        "\t-j, --json         run with arguments in JSON format\n"
        "\t-s, --seed         seed for the number generator\n"
        "\t-r, --rows         rows [max: 100]\n"
        "\t-o, --output       output format\n"
        "\t                     [json, obj, text, stdout]\n"
        "\t-v, --version      display program version\n";
}

std::string cli::m_help_str = get_cli_help_str();

std::string cli::m_title_str = get_cli_title_str();

std::string cli::m_version_str = get_cli_version_str();

std::string cli::convert(std::vector<std::string> const& args_vec) const noexcept
{
#if defined(MAZE_DEBUG)

    m_version_str += " - DEBUG";
#endif

    if (args_vec.empty())
    {
        return m_help_str;
    }

    try
    {
        if (const auto need_help = std::ranges::find_if(args_vec, [](const std::string& arg)
        {
            return arg == mazes::args::HELP_FLAG_STR || arg == mazes::args::HELP_OPTION_STR || arg ==
                mazes::args::HELP_WORD_STR;
        }); need_help != args_vec.cend())
        {
            return m_help_str;
        }

        if (const auto need_version = std::ranges::find_if(args_vec, [](const std::string& arg)
        {
            return arg == mazes::args::VERSION_FLAG_STR || arg == mazes::args::VERSION_OPTION_STR || arg ==
                mazes::args::VERSION_WORD_STR;
        }); need_version != args_vec.cend())
        {
            return m_version_str;
        }

        mazes::configurator user_options;
        return this->convert_with_options(std::cref(args_vec), std::ref(user_options));
    }
    catch (const std::exception& ex)
    {
        std::cerr << "CLI convert error: " << ex.what() << std::endl;
    }

    return "";
} // convert

std::string cli::convert_with_options(std::vector<std::string> const& args_vec, mazes::configurator& user_options) noexcept
{
    try
    {
        if (!config_mapper::map_args_to_config(std::cref(args_vec), std::ref(user_options)))
        {
            return "";
        }
    }
    catch (std::exception& ex)
    {
        return std::string("Configuration Error: ") + ex.what();
    }

    mazes::grid_factory factory;

    factory.register_creator(
        m_title_str, [](const mazes::configurator& config) -> std::unique_ptr<mazes::grid_interface>
        {
            return std::make_unique<mazes::distance_grid>(config.rows(), config.columns(), config.levels());
        });

    if (const auto product = factory.create(m_title_str, std::cref(user_options));
        product.has_value())
    {
        mazes::randomizer rng;

        apply(product.value().get(), rng, user_options.algo_id(), std::cref(user_options));

        const mazes::stringify stringifier;

        // Check if we need to generate Wavefront OBJ output
        if (user_options.output_format_id() == mazes::output_format::WAVEFRONT_OBJECT_FILE)
        {
            // Execute the stringify algorithm on the grid product
            if (!stringifier.run(product.value().get(), rng))
            {
                return "Failed to stringify";
            }

            // Generate 3D object data
            if (const mazes::objectify o; !o.run(product.value().get(), rng))
            {
                return "Failed to objectify";
            }

            // Convert to Wavefront OBJ format
            if (const mazes::wavefront_object_helper w; !w.run(product.value().get(), std::ref(rng)))
            {
                return "Failed to generate Wavefront OBJ data.";
            }
        }
        else
        {
            if (const mazes::stringify s; !s.run(product.value().get(), rng))
            {
                return "Failed to stringify";
            }
        }

        return product.value()->operations().get_str();
    }

    return "";
}

std::string cli::convert_as_base64(std::vector<std::string> const& args_vec) const noexcept
{
    return mazes::bytes::encode(convert(std::cref(args_vec)));
}

std::string cli::help() noexcept
{
    return m_help_str;
}

std::string cli::version() noexcept
{
    return m_version_str;
}

/// @brief Apply an algorithm to the grid
/// @param g
/// @param rng
/// @param a
/// @param config
void cli::apply(mazes::grid_interface* g, mazes::randomizer& rng, const mazes::algo a,
                const mazes::configurator& config) noexcept
{
    try
    {
        bool success = false;
        switch (a)
        {
        case mazes::algo::BINARY_TREE:
            {
                static mazes::binary_tree bt;
                success = bt.run(g, std::ref(rng));
                break;
            }
        case mazes::algo::SIDEWINDER:
            {
                static mazes::sidewinder sw;
                success = sw.run(g, std::ref(rng));
                break;
            }
        case mazes::algo::DFS:
            {
                static mazes::dfs d;
                success = d.run(g, std::ref(rng));
                break;
            }
        default:
            throw std::runtime_error("Unsupported algorithm: " + std::string{mazes::to_sv_from_algo(a)});
        } // switch

        if (!success)
        {
            throw std::runtime_error("Algo failed to run: " + std::string{mazes::to_sv_from_algo(a)});
        }

        // Calculate distances after maze generation if requested
        if (config.distances())
        {
            // Try to cast to distance_grid to call calculate_distances
            if (const auto distance_grid_ptr = dynamic_cast<mazes::distance_grid*>(g))
            {
                int start_idx = config.distances_start();
                int end_idx = config.distances_end();

                // If end index is -1 (default), use the last cell
                const int max_cell_index = (config.rows() * config.columns()) - 1;

                // Resolve negative end indices by counting backwards from the last cell:
                // -1 -> last cell (max_cell_index), -2 -> second-to-last, etc.
                const auto resolve_end_idx = [max_cell_index](int given_end) noexcept -> int {
                    if (given_end >= 0) {
                        return given_end;
                    }
                    // Translate negative index into 0-based index from the end
                    // Example: given_end == -1 -> (max_cell_index + 1) - 1 == max_cell_index
                    return (max_cell_index + 1) + given_end;
                };

                end_idx = resolve_end_idx(end_idx);

                // Ensure indices are within valid range
                start_idx = std::clamp(start_idx, 0, max_cell_index);
                end_idx = std::clamp(end_idx, 0, max_cell_index);
                distance_grid_ptr->calculate_distances(start_idx, end_idx);

                if (const auto distances = distance_grid_ptr->get_distances(); !distances)
                {
                    throw std::runtime_error("Distances object is null after calculation.");
                }
            }
            else
            {
                throw std::runtime_error{std::string{mazes::to_sv_from_algo(a)}};
            }
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "CLI apply failed: " << ex.what() << std::endl;
    } // catch
} // apply
