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

#include <fmt/format.h>

namespace mazes::state_utils
{
    inline void parse_dimensions(const std::optional<args> &args, unsigned int &rows, unsigned int &cols,
                                 unsigned int &levels) noexcept
    {
        if (const auto parsed = args ? args->get() : std::nullopt; parsed.has_value())
        {
            if (const auto it = parsed->find(mazes::args::ROW_WORD_STR); it != parsed->cend())
            {
                try
                {
                    rows = static_cast<unsigned int>(std::stoul(it->second));
                }
                catch (...)
                {
                }
            }
            if (const auto it = parsed->find(mazes::args::COLUMN_WORD_STR); it != parsed->cend())
            {
                try
                {
                    cols = static_cast<unsigned int>(std::stoul(it->second));
                }
                catch (...)
                {
                }
            }
            if (const auto it = parsed->find(mazes::args::LEVEL_WORD_STR); it != parsed->cend())
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
    }

    inline bool has_distances(const std::optional<args> &args) noexcept
    {
        if (const auto parsed = args ? args->get() : std::nullopt; parsed.has_value())
        {
            return parsed->find(mazes::args::DISTANCES_WORD_STR) != parsed->cend();
        }
        return false;
    }

    struct distance_settings final
    {
        bool enabled{false};
        int start{configurator::DEFAULT_DISTANCES_START};
        int end{configurator::DEFAULT_DISTANCES_END};
    };

    inline distance_settings parse_distance_settings(const std::optional<args> &args) noexcept
    {
        distance_settings settings{};

        if (const auto parsed = args ? args->get() : std::nullopt; parsed.has_value())
        {
            settings.enabled = parsed->find(mazes::args::DISTANCES_WORD_STR) != parsed->cend();

            if (const auto it = parsed->find(mazes::args::DISTANCES_START_VAL_STR); it != parsed->cend())
            {
                try
                {
                    settings.start = std::stoi(it->second);
                }
                catch (...)
                {
                }
            }
            if (const auto it = parsed->find(mazes::args::DISTANCES_END_VAL_STR); it != parsed->cend())
            {
                try
                {
                    settings.end = std::stoi(it->second);
                }
                catch (...)
                {
                }
            }
        }

        return settings;
    }

    inline state::ID output_state_for(const std::optional<args> &args) noexcept
    {
        if (const auto parsed = args ? args->get() : std::nullopt; parsed.has_value())
        {
            if (const auto it = parsed->find(mazes::args::OUTPUT_ID_WORD_STR); it != parsed->cend())
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
        }

        return state::ID::STRINGIFYING;
    }

    inline std::optional<args> get_args(const runtime_app::context &ctx) noexcept
    {
        if (auto *args_mapper = ctx.get_args_manager())
        {
            try
            {
                return args_mapper->get(args_identifier::PARSED);
            }
            catch (...)
            {
            }
        }
        return std::nullopt;
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
