#ifndef STATE_UTILS_H
#define STATE_UTILS_H

#include <MazeBuilder/args.h>
#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/output_formats.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/state.h>
#include <MazeBuilder/string_utils.h>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_map>

/// @brief Namespace containing utility functions for maze builder states
/// @file state_utils.h
namespace mazes::state_utils
{
    namespace details
    {
        inline state::ID to_state_from_output_format(output_format of) noexcept
        {
            switch (of)
            {
            case output_format::JPG:
            case output_format::JPEG:
            case output_format::PNG:
                return state::ID::WRITE_TO_IMAGE;
            case output_format::PLAIN_TEXT:
            case output_format::PLAIN_TEXT_ALT:
            case output_format::JSON:
            case output_format::STDOUT:
                return state::ID::WRITE_TO_STRING;
            case output_format::OBJ:
                return state::ID::WRITE_TO_WF_OBJ;
            default:
                return state::ID::EMPTY;
            }
        }
    }

    /// @brief Parses the maze dimensions from the given arguments.
    /// @param args The arguments containing the dimension information.
    /// @param rows Reference to store the number of rows.
    /// @param cols Reference to store the number of columns.
    /// @param levels Reference to store the number of levels.
    inline void parse_dimensions(const std::unordered_map<std::string, std::string>& args, unsigned int& rows, unsigned int& cols,
        unsigned int& levels) noexcept
    {
        if (const auto it = args.find(mazes::args::ROW_WORD_STR); it != args.cend())
        {
            try
            {
                rows = static_cast<unsigned int>(std::stoul(it->second));
            } catch (...)
            {
            }
        }
        if (const auto it = args.find(mazes::args::COLUMN_WORD_STR); it != args.cend())
        {
            try
            {
                cols = static_cast<unsigned int>(std::stoul(it->second));
            } catch (...)
            {
            }
        }
        if (const auto it = args.find(mazes::args::LEVEL_WORD_STR); it != args.cend())
        {
            try
            {
                levels = static_cast<unsigned int>(std::stoul(it->second));
            } catch (...)
            {
            }
        }
    }

    /// @brief Struct to hold distance settings parsed from arguments
    struct distance_settings final
    {
        bool enabled{ false };
        int start{ configurator::DEFAULT_DISTANCES_START };
        int end{ configurator::DEFAULT_DISTANCES_END };
    };

    /// @brief Parses the distance settings from the given arguments.
    /// @param args The arguments containing the distance settings.
    /// @return A distance_settings struct populated with the parsed values.
    inline distance_settings parse_distance_settings(const std::unordered_map<std::string, std::string>& args) noexcept
    {
        distance_settings settings{};

        settings.enabled = args.find(mazes::args::DISTANCES_WORD_STR) != args.cend();

        if (const auto it = args.find(mazes::args::DISTANCES_START_VAL_STR); it != args.cend())
        {
            try
            {
                settings.start = std::stoi(it->second);
            } catch (...)
            {
            }
        }
        if (const auto it = args.find(mazes::args::DISTANCES_END_VAL_STR); it != args.cend())
        {
            try
            {
                settings.end = std::stoi(it->second);
            } catch (...)
            {
            }
        }

        return settings;
    }

    /// @brief Checks if the "distances" argument is present in the given arguments.
/// @details This function checks if the "distances" argument is present in the provided
/// @param args
/// @return
    inline bool has_distances(const std::unordered_map<std::string, std::string>& args) noexcept
    {
        return parse_distance_settings(args).enabled;
    }

    /// @brief Determines the output state based on the given arguments.
    /// @param args The arguments containing the output information.
    /// @return The corresponding state::ID for the output.
    inline state::ID output_state_for(const std::unordered_map<std::string, std::string>& args) noexcept
    {
        state::ID output_state = state::ID::EMPTY;
        if (const auto it = args.find(mazes::args::OUTPUT_ID_WORD_STR); it != args.cend())
        {
            const auto& output = string_utils::file_extension(it->second);
            output_state = details::to_state_from_output_format(output_format_or_default(output));
        }

        return output_state;
    }

    inline std::unordered_map<std::string, std::string> get_args_at_front(const runtime_app::context& ctx) noexcept
    {
        if (auto* args_mapper = ctx.get_args_manager())
        {
            try
            {
                auto&& parsed_args = args_mapper->get(args_identifier::PARSED).front();

                if (parsed_args.empty())
                {
                    parsed_args = args_mapper->get(args_identifier::RAW).front();
                }
                return parsed_args;
            } catch (...)
            {
            }
        }
        return {};
    }

    inline bool advance_args(const runtime_app::context& ctx) noexcept
    {
        if (auto* args_mapper = ctx.get_args_manager())
        {
            try
            {
                auto& parsed_args = args_mapper->get(args_identifier::PARSED);
                return parsed_args.pop_front() && parsed_args.count() > 0;
            } catch (...)
            {
            }
        }
        return false;
    }

    inline randomizer* get_rng_or_default(const runtime_app::context& ctx) noexcept
    {
        if (auto* rng = ctx.get_rng())
        {
            return rng;
        }

        // Thread-local storage keeps the fallback lifetime valid for callers.
        static thread_local randomizer fallback_rng{};
        return &fallback_rng;
    }

    // Reseeds rng from the parsed "seed" arg so repeated apply() calls with the same
    // seed reproduce identical topology; the rng is otherwise shared and keeps advancing.
    inline void reseed_from_args(const std::unordered_map<std::string, std::string>& args, randomizer& rng) noexcept
    {
        if (const auto it = args.find(mazes::args::SEED_WORD_STR); it != args.cend())
        {
            try
            {
                rng.seed(std::stoull(it->second));
            } catch (...)
            {
            }
        }
    }

    template <typename... Mappers>
    inline void validate_mappers(Mappers &&...mapper) noexcept
    {
        if (!((mapper != nullptr) && ...))
        {
            global_async_logger().log("mappers are null");
        }
    }

    /// @brief Resets the grid's topology (links) before each generation.
    /// @details resize() always clears cell links even when dimensions are unchanged;
    /// without this, repeated maze generation at the same size (e.g. rebuilding on
    /// keypress) would keep linking new random walls on top of the previous maze's
    /// links, so the maze gradually loses walls / becomes fully open over time.
    /// @param ops Pointer to the grid operations.
    /// @param rows Desired number of rows.
    /// @param cols Desired number of columns.
    /// @param levels Desired number of levels.
    inline void check_before_resize(grid_operations* ops, unsigned int rows, unsigned int cols, unsigned int levels) noexcept
    {
        if (ops == nullptr)
        {
            global_async_logger().log("grid_operations pointer is null");
            return;
        }

        ops->resize(rows, cols, levels);
    }

} // namespace mazes::state_utils

#endif // STATE_UTILS_H
