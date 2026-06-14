#ifndef OUTPUT_FORMATS_H
#define OUTPUT_FORMATS_H

#include <stdexcept>
#include <string>
#include <string_view>

/// @namespace mazes
/// @file output_formats.h
namespace mazes
{
    namespace
    {
        constexpr std::string_view JSON_FILE_FORMAT_STR = "json";
        constexpr std::string_view WAVEFRONT_OBJECT_FILE_FORMAT_STR = "obj";
        constexpr std::string_view STDOUT_FORMAT_STR = "stdout";
        constexpr std::string_view PLAIN_TEXT_FORMAT_STR = "txt";
    }

    /// @brief Enum class for output_format types
    enum class output_format : unsigned int
    {
        PLAIN_TEXT = 0,
        JSON_FILE = 1,
        WAVEFRONT_OBJECT_FILE = 2,
        STDOUT = 3,
        TOTAL = 4
    };

    /// @brief Convert an output_format enum to a string
    /// @param of
    /// @return
    inline std::string_view to_sv_from_output_format(output_format of)
    {
        switch (of)
        {
        case output_format::PLAIN_TEXT:
            return PLAIN_TEXT_FORMAT_STR;
        case output_format::JSON_FILE:
            return JSON_FILE_FORMAT_STR;
        case output_format::WAVEFRONT_OBJECT_FILE:
            return WAVEFRONT_OBJECT_FILE_FORMAT_STR;
        case output_format::STDOUT:
            return STDOUT_FORMAT_STR;
        default:
            throw std::invalid_argument("Invalid output_format: " + std::to_string(static_cast<unsigned int>(of)));
        }
    };

    /// @brief Convert a string to an output_format enum
    /// @param sv
    /// @return
    inline output_format to_output_format_from_sv(std::string_view sv)
    {
        if (sv.compare(PLAIN_TEXT_FORMAT_STR) == 0)
        {
            return output_format::PLAIN_TEXT;
        }
        else if (sv.compare(JSON_FILE_FORMAT_STR) == 0)
        {
            return output_format::JSON_FILE;
        }
        else if (sv.compare(WAVEFRONT_OBJECT_FILE_FORMAT_STR) == 0)
        {
            return output_format::WAVEFRONT_OBJECT_FILE;
        }
        else if (sv.compare(STDOUT_FORMAT_STR) == 0)
        {
            return output_format::STDOUT;
        }
        else
        {
            throw std::invalid_argument("Invalid output_format: " + std::string{sv});
        }
    };

} // namespace mazes

#endif // OUTPUT_FORMATS_H
