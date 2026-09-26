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
    /// @return A formatted string view wrapped in double quotes with escaped newlines
    inline std::string_view format_maze_from_mask(const std::string_view mask_string) noexcept
    {
        thread_local std::string formatted_mask{};
        formatted_mask.clear();
        formatted_mask.reserve(mask_string.size() + 2);
        formatted_mask.push_back('"');

        for (const char ch : mask_string)
        {
            if (ch == '\r')
            {
                continue;
            }

            switch (ch)
            {
            case '\n':
                formatted_mask += "\\n";
                break;
            case '\\':
                formatted_mask += "\\\\";
                break;
            case '"':
                formatted_mask += "\\\"";
                break;
            default:
                formatted_mask += fmt::format("{}", ch);
                break;
            }
        }

        formatted_mask.push_back('"');
        return formatted_mask;
    }
} // namespace mazes

#endif // FORMAT_MAZE_FROM_MASK_H
