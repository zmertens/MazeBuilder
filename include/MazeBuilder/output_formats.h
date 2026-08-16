#ifndef OUTPUT_FORMATS_H
#define OUTPUT_FORMATS_H

#include <array>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>

/// @namespace mazes
/// @file output_formats.h
namespace mazes
{
    /// @brief Enum class for output_format types
    enum class output_format : unsigned int
    {
        JPEG = 0,
        PLAIN_TEXT = 1,
        PNG = 2,
        JSON = 3,
        OBJ = 4,
        STDOUT = 5,
        TOTAL = 6
    };

    constexpr std::array<std::string_view, static_cast<size_t>(output_format::TOTAL)> OUTPUT_FORMAT_LABELS_LOWERCASE = {
        "jpeg",
        "txt",
        "png",
        "json",
        "obj",
        "stdout"};

    /// @brief Convert an output_format enum to a string
    /// @param of
    /// @return
    inline std::string_view to_sv_from_output_format(output_format of)
    {
        switch (of)
        {
        case output_format::JPEG:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[0];
        case output_format::PLAIN_TEXT:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[1];
        case output_format::PNG:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[2];
        case output_format::JSON:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[3];
        case output_format::OBJ:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[4];
        case output_format::STDOUT:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[5];
        default:
            throw std::invalid_argument("Invalid output_format: " + std::to_string(static_cast<unsigned int>(of)));
        }
    }

    inline output_format output_format_or_default(const std::string_view sv,
                                                 const output_format fallback = output_format::PLAIN_TEXT) noexcept
    {
        if (sv.empty())
        {
            return fallback;
        }

        std::string normalized;
        normalized.reserve(sv.size());
        for (const char ch : sv)
        {
            normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }

        if (normalized.starts_with('.'))
        {
            normalized.erase(0, 1);
        }

        if (normalized.empty() || normalized == "stdout")
        {
            return output_format::STDOUT;
        }

        if (normalized == "txt" || normalized == "text")
        {
            return output_format::PLAIN_TEXT;
        }

        if (normalized == "png")
        {
            return output_format::PNG;
        }

        if (normalized == "jpg" || normalized == "jpeg")
        {
            return output_format::JPEG;
        }

        if (normalized == "json")
        {
            return output_format::JSON;
        }

        if (normalized == "obj")
        {
            return output_format::OBJ;
        }

        return fallback;
    }

    /// @brief Convert a string to an output_format enum
    /// @param sv
    /// @return
    inline output_format to_output_format_from_sv(const std::string_view sv)
    {
        if (const auto it = std::ranges::find(OUTPUT_FORMAT_LABELS_LOWERCASE, sv); it != OUTPUT_FORMAT_LABELS_LOWERCASE.cend())
        {
            const auto index = static_cast<size_t>(std::distance(OUTPUT_FORMAT_LABELS_LOWERCASE.cbegin(), it));
            switch (index)
            {
            case 0:
                return output_format::JPEG;
            case 1:
                return output_format::PLAIN_TEXT;
            case 2:
                return output_format::PNG;
            case 3:
                return output_format::JSON;
            case 4:
                return output_format::OBJ;
            case 5:
                return output_format::STDOUT;
            default:
                break;
            }
        }
        throw std::invalid_argument("Invalid output_format: " + std::string{sv});
    }
} // namespace mazes

#endif // OUTPUT_FORMATS_H
