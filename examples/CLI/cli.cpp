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
        "Example: ./cli -r 10 -c 10 -a binary_tree > maze.txt\n\n"
        "Example: ./cli --rows=10 --columns=10 --algo=dfs -o maze.obj\n\n"
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
        "\t                     [jpg, json, obj, png, text, stdout]\n"
        "\t-v, --version      display program version\n";
}

std::string cli::m_debug_str;

std::string cli::m_help_str = get_cli_help_str();

std::string cli::m_title_str = get_cli_title_str();

std::string cli::m_version_str = get_cli_version_str();

std::string cli::convert(std::vector<std::string> const& args_vec) const noexcept
{
#if defined(MAZE_DEBUG)

    m_debug_str = m_version_str + " - DEBUG";
#endif

    if (args_vec.empty())
    {
        return m_help_str;
    }

    try
    {
        if (auto need_help = std::ranges::find_if(args_vec, [](const std::string& arg)
        {
            return arg == mazes::args::HELP_FLAG_STR || arg == mazes::args::HELP_OPTION_STR || arg ==
                mazes::args::HELP_WORD_STR;
        }); need_help != args_vec.cend())
        {
            return m_help_str;
        }

        if (auto need_version = std::ranges::find_if(args_vec, [](const std::string& arg)
        {
            return arg == mazes::args::VERSION_FLAG_STR || arg == mazes::args::VERSION_OPTION_STR || arg ==
                mazes::args::VERSION_WORD_STR;
        }); need_version != args_vec.cend())
        {
#if defined(MAZE_DEBUG)

            return m_debug_str;
#else

            return m_version_str;
#endif
        }

        mazes::configurator user_options;
        return this->convert(std::cref(args_vec), std::ref(user_options));
    }
    catch (const std::exception& ex)
    {
#if defined(MAZE_DEBUG)

        std::cerr << "CLI Error: " << ex.what() << std::endl;
#endif
    }

    return "";
} // convert

std::string cli::convert(std::vector<std::string> const& args_vec, mazes::configurator& user_options) const noexcept
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
            auto vertices = product.value()->operations().get_vertices();
            auto faces = product.value()->operations().get_faces();

            if (const mazes::wavefront_object_helper w; !w.run(product.value().get(), std::ref(rng)))
            {
                return "Failed to generate Wavefront OBJ data.";
            }
        }
        else if (user_options.output_format_id() == mazes::output_format::PNG ||
            user_options.output_format_id() == mazes::output_format::JPEG)
        {
            // PNG/JPEG export
            // First run stringify to get ASCII representation
            if (!stringifier.run(product.value().get(), rng))
            {
                return "Failed to stringify";
            }

            // Run pixels algorithm to convert ASCII to pixel data
            if (mazes::pixels pixelizer; !pixelizer.run(product.value().get(), rng))
            {
                return "Failed to generate pixel data.";
            }

            // Get pixel vector and convert to string for transmission/storage
            auto pixel_vec = product.value()->operations().get_pixels();

            // Compute and store image size into configurator
            compute_and_store_image_size(product.value().get(), user_options);

            // Convert bytes to string
            auto image_data_str = bytes_to_string(pixel_vec);

            // Replace the previous return of ASCII with the raw image bytes (in string form)
            return image_data_str;
        }
        else
        {
            if (mazes::stringify s; !s.run(product.value().get(), rng))
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
    return mazes::base64_helper::encode(convert(std::cref(args_vec)));
}

std::string cli::help() noexcept
{
    return m_help_str;
}

std::string cli::version() noexcept
{
    return m_version_str;
}

std::string cli::bytes_to_string(const std::vector<std::uint8_t>& bytes)
{
    if (bytes.empty())
    {
        return std::string{};
    }

    std::string s;
    s.resize(bytes.size());
    // Range-based copy
    std::ranges::copy(bytes, reinterpret_cast<std::uint8_t*>(s.data()));
    return s;
}

std::vector<std::uint8_t> cli::string_to_bytes(const std::string& s)
{
    if (s.empty())
    {
        return {};
    }

    std::vector<std::uint8_t> v;
    v.resize(s.size());
    std::copy_n(reinterpret_cast<const std::uint8_t*>(s.data()),
              s.size(),
              v.begin());
    return v;
}

// Helper: compute image dimensions and store into configurator
void cli::compute_and_store_image_size(const mazes::grid_interface* g, mazes::configurator& cfg) noexcept
{
    if (g == nullptr)
    {
        return;
    }

    const auto [rows, cols, _] = g->operations().get_dimensions();

    // Compute a "reasonable" scale based on grid area. Use sqrt(rows*cols) rounded up, but at least 1.
    const double area = static_cast<double>(rows) * static_cast<double>(cols);
    const unsigned int scale = static_cast<unsigned int>(std::max(1.0, std::ceil(std::sqrt(area))));

    // Image width and height in pixels - assume each cell is 'scale' pixels square, and include border lines.
    // For ASCII-art style maze (using corner and barrier characters), each cell maps to (scale) pixels, but
    // walls/borders add 1 pixel per boundary; to be conservative, compute: width = cols * scale + (cols + 1);
    const unsigned int width = cols * scale + (cols + 1);
    const unsigned int height = rows * scale + (rows + 1);

    cfg.image_width(width);
    cfg.image_height(height);
}

/// @brief Apply an algorithm to the grid
/// @param g
/// @param rng
/// @param a
/// @param config
void cli::apply(mazes::grid_interface* g, mazes::randomizer& rng, const mazes::algo a,
                const mazes::configurator& config) noexcept
{
    using namespace std;

    try
    {
        bool success = false;

        switch (a)
        {
        case mazes::algo::BINARY_TREE:
            {
                static mazes::binary_tree bt;

                success = bt.run(g, ref(rng));

                break;
            }
        case mazes::algo::SIDEWINDER:
            {
                static mazes::sidewinder sw;

                success = sw.run(g, ref(rng));

                break;
            }
        case mazes::algo::DFS:
            {
                static mazes::dfs d;

                success = d.run(g, ref(rng));

                break;
            }
        default:

            throw std::invalid_argument("Unsupported algorithm: " + std::string{mazes::to_sv_from_algo(a)});
        } // switch

        if (!success)
        {
            throw std::runtime_error("Failed to run algorithm: " + std::string{mazes::to_sv_from_algo(a)});
        }

        // Calculate distances after maze generation if requested
        if (config.distances())
        {
            // Try to cast to distance_grid to call calculate_distances
            if (auto distance_grid_ptr = dynamic_cast<mazes::distance_grid*>(g); distance_grid_ptr != nullptr)
            {
                int start_idx = config.distances_start();
                int end_idx = config.distances_end();

                // If end index is -1 (default), use the last cell
                if (end_idx == -1)
                {
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

                if (auto distances = distance_grid_ptr->get_distances())
                {
                    std::cerr << "Debug: Distances object created successfully" << std::endl;
                }
                else
                {
                    std::cerr << "Debug: Failed to create distances object" << std::endl;
                }
#endif
            }
            else
            {
#if defined(MAZE_DEBUG)

                std::cerr << "Debug: Failed to calculate distances" << std::endl;
#endif
            }
        }
    }
    catch (const std::exception& ex)
    {
#if defined(MAZE_DEBUG)

        std::cerr << "Algorithm Error: " << ex.what() << std::endl;
#endif
    } // catch
} // apply
