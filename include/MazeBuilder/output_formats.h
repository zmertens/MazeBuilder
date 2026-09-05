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
        JPG = 0,
        JPEG = 1,
        PLAIN_TEXT = 2,
        PLAIN_TEXT_ALT = 3,
        PNG = 4,
        JSON = 5,
        OBJ = 6,
        STDOUT = 7,
        TOTAL = 8
    };

    constexpr std::array<std::string_view, static_cast<size_t>(output_format::TOTAL)> OUTPUT_FORMAT_LABELS_LOWERCASE = {
        "jpg",
        "jpeg",
        "text",
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
        case output_format::JPG:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[0];
        case output_format::JPEG:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[1];
        case output_format::PLAIN_TEXT:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[2];
        case output_format::PLAIN_TEXT_ALT:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[3];
        case output_format::PNG:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[4];
        case output_format::JSON:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[5];
        case output_format::OBJ:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[6];
        case output_format::STDOUT:
            return OUTPUT_FORMAT_LABELS_LOWERCASE[7];
        default:
            throw std::invalid_argument("Invalid output_format: " + std::to_string(static_cast<unsigned int>(of)));
        }
    }

    /// @brief Convert a string to an output_format enum
    /// @param sv
    /// @return
    inline output_format to_output_format_from_sv(const std::string_view sv)
    {
        if (sv.empty())
        {
            throw std::invalid_argument("Invalid output_format: empty");
        }

        std::string normalized{};
        normalized.reserve(sv.size());
        for (const unsigned char ch : sv)
        {
            normalized.push_back(static_cast<char>(std::tolower(ch)));
        }

        if (const auto it = std::ranges::find(OUTPUT_FORMAT_LABELS_LOWERCASE, normalized); it != OUTPUT_FORMAT_LABELS_LOWERCASE.cend())
        {
            const auto index = static_cast<size_t>(std::distance(OUTPUT_FORMAT_LABELS_LOWERCASE.cbegin(), it));
            switch (index)
            {
            case 0:
                return output_format::JPG;
            case 1:
                return output_format::JPEG;
            case 2:
                return output_format::PLAIN_TEXT;
            case 3:
                return output_format::PLAIN_TEXT_ALT;
            case 4:
                return output_format::PNG;
            case 5:
                return output_format::JSON;
            case 6:
                return output_format::OBJ;
            case 7:
                return output_format::STDOUT;
            case 8:
                return output_format::TOTAL;
            default:
                break;
            }
        }
        throw std::invalid_argument("Invalid output_format: " + std::string{sv});
    }

    /// @brief Converts a string to an output_format enum, or returns a fallback if invalid.
    /// @param sv The string representation of the output format.
    /// @param fallback The fallback output_format to return if the string is invalid.
    /// @return The corresponding output_format enum, or the fallback if invalid.
    inline output_format output_format_or_default(const std::string_view sv,
                                                  const output_format fallback = output_format::PLAIN_TEXT) noexcept
    {
        if (sv.empty())
        {
            return fallback;
        }

        try
        {
            return to_output_format_from_sv(sv);
        }
        catch (...)
        {
            return fallback;
        }
    }
} // namespace mazes

#endif // OUTPUT_FORMATS_H
