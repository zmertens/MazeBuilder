#ifndef STATE_UTILS_H
#define STATE_UTILS_H

#include <MazeBuilder/args.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/output_formats.h>
#include <MazeBuilder/state.h>

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

#include <fmt/format.h>

namespace mazes::state_utils
{
    inline void parse_dimensions(const std::unordered_map<std::string, std::string> &args, unsigned int &rows, unsigned int &cols,
                                 unsigned int &levels) noexcept
    {
        if (const auto it = args.find(mazes::args::ROW_WORD_STR); it != args.cend())
        {
            try
            {
                rows = static_cast<unsigned int>(std::stoul(it->second));
            }
            catch (...)
            {
            }
        }
        if (const auto it = args.find(mazes::args::COLUMN_WORD_STR); it != args.cend())
        {
            try
            {
                cols = static_cast<unsigned int>(std::stoul(it->second));
            }
            catch (...)
            {
            }
        }
        if (const auto it = args.find(mazes::args::LEVEL_WORD_STR); it != args.cend())
        {
            try
            {
                levels = static_cast<unsigned int>(std::stoul(it->second));
            }
            catch (...)
            {
            }
        }
    }

    inline bool has_distances(const std::unordered_map<std::string, std::string> &args) noexcept
    {
        if (!args.empty())
        {
            return args.find(mazes::args::DISTANCES_WORD_STR) != args.cend();
        }
        return false;
    }

    struct distance_settings final
    {
        bool enabled{false};
        int start{configurator::DEFAULT_DISTANCES_START};
        int end{configurator::DEFAULT_DISTANCES_END};
    };

    inline distance_settings parse_distance_settings(const std::unordered_map<std::string, std::string> &args) noexcept
    {
        distance_settings settings{};;

        settings.enabled = args.find(mazes::args::DISTANCES_WORD_STR) != args.cend();

        if (const auto it = args.find(mazes::args::DISTANCES_START_VAL_STR); it != args.cend())
        {
            try
            {
                settings.start = std::stoi(it->second);
            }
            catch (...)
            {
            }
        }
        if (const auto it = args.find(mazes::args::DISTANCES_END_VAL_STR); it != args.cend())
        {
            try
            {
                settings.end = std::stoi(it->second);
            }
            catch (...)
            {
            }
        }

        return settings;
    }

    inline state::ID output_state_for(const std::unordered_map<std::string, std::string> &args) noexcept
    {
        if (const auto it = args.find(mazes::args::OUTPUT_ID_WORD_STR); it != args.cend())
        {
            const std::string output = it->second;
            if (output.empty() || output == "stdout")
            {
                return state::ID::STRINGIFYING;
            }

            const auto ext = std::filesystem::path{output}.extension().string();
            if (ext.empty())
            {
                return state::ID::STRINGIFYING;
            }

            auto normalized = ext;
            if (!normalized.empty() && normalized.front() == '.')
            {
                normalized.erase(normalized.begin());
            }

            if (normalized == "txt" || normalized == "stdout")
            {
                return state::ID::STRINGIFYING;
            }

            if (normalized == "json")
            {
                return state::ID::PARSING;
            }

            if (normalized == "obj")
            {
                return state::ID::WAVEFRONT_OBJECTIFY;
            }

            if (normalized == "png" || normalized == "jpg" || normalized == "jpeg" || normalized == "bmp")
            {
                return state::ID::PIXELIZING;
            }
        }

        return state::ID::STRINGIFYING;
    }

    inline std::unordered_map<std::string, std::string> get_args_at_front(const runtime_app::context &ctx) noexcept
    {
        if (auto *args_mapper = ctx.get_args_manager())
        {
            try
            {
                auto &&parsed_args = args_mapper->get(args_identifier::PARSED).front();

                if (parsed_args.empty())
                {
                    parsed_args = args_mapper->get(args_identifier::RAW).front();
                }
                return parsed_args;
            }
            catch (...)
            {
            }
        }
        return {};
    }

    inline bool advance_args(const runtime_app::context &ctx) noexcept
    {
        if (auto *args_mapper = ctx.get_args_manager())
        {
            try
            {
                auto &parsed_args = args_mapper->get(args_identifier::PARSED);
                return parsed_args.pop_front() && parsed_args.count() > 0;
            }
            catch (...)
            {
            }
        }
        return false;
    }

    inline randomizer *get_rng_or_default(const runtime_app::context &ctx) noexcept
    {
        if (auto *rng = ctx.get_rng())
        {
            return rng;
        }

        // Thread-local storage keeps the fallback lifetime valid for callers.
        static thread_local randomizer fallback_rng{};
        return &fallback_rng;
    }

    template <typename... Mappers>
    inline void validate_mappers(Mappers &&...mapper) noexcept
    {
        if (!((mapper != nullptr) && ...))
        {
            global_async_logger().log("mappers are null");
        }
    }
} // namespace mazes::state_utils

#endif // STATE_UTILS_H
