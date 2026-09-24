#ifndef FORMAT_MAZE_FROM_MASK_H
#define FORMAT_MAZE_FROM_MASK_H

#include <string>
#include <string_view>

#include <fmt/format.h>

/// @file format_maze_from_mask.h
/// @namespace mazes
namespace mazes
{
    /// @brief Format a raw mask string for inline CLI use with -m/--mask
    /// @param mask_string Raw mask text that may contain embedded newlines
    /// @return A formatted string view wrapped in backticks with escaped newlines
    inline std::string_view format_maze_from_mask(const std::string_view mask_string) noexcept
    {
        thread_local std::string formatted_mask{};
        thread_local std::string escaped_mask{};

        escaped_mask.clear();
        escaped_mask.reserve(mask_string.size());

        for (const char ch : mask_string)
        {
            if (ch == '\r')
            {
                continue;
            }

            escaped_mask += (ch == '\n') ? "\\n" : fmt::format("{}", ch);
        }

        formatted_mask = fmt::format("`{}`", escaped_mask);
        return formatted_mask;
    }
} // namespace mazes

#endif // FORMAT_MAZE_FROM_MASK_H
