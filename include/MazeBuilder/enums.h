#ifndef ENUMS
#define ENUMS

#include <cstdint>
#include <string>
#include <string_view>
#include <stdexcept>

/// @file enums.h
/// @brief Enumerations and utilities for the maze builder program

namespace mazes {

    /// @brief Character representations of walls and barriers in the maze
    enum class barriers : char {

        HORIZONTAL = '-',

        VERTICAL = '|',

        CORNER = '+',

        SINGLE_SPACE = ' '
    };

    /// @brief Enum class for output_format types
    enum class output_format : unsigned int {

        PLAIN_TEXT = 0,

        JSON = 1,

        WAVEFRONT_OBJECT_FILE = 2,

        PNG = 3,

        JPEG = 4,

        STDOUT = 5,

        TOTAL = 6
    };

    /// @brief Convert an output_format enum to a string
    /// @param o
    /// @return 
    inline std::string_view to_string_from_output_format(output_format of) {

        switch (of) {

        case output_format::PLAIN_TEXT:

            return "txt";
        case output_format::JSON:

            return "json";
        case output_format::WAVEFRONT_OBJECT_FILE:

            return "obj";
        case output_format::PNG:

            return "png";
        case output_format::JPEG:

            return "jpeg";
        case output_format::STDOUT:

            return "stdout";
        default:

            throw std::invalid_argument("Invalid output_format: " + std::to_string(static_cast<unsigned int>(of)));
        }
    };

    /// @brief Convert a string to an output_format enum
    /// @param o
    /// @return
    inline output_format to_output_format_from_string(std::string_view sv) {
        
        if (sv.compare("txt") == 0) {

            return output_format::PLAIN_TEXT;
        } else if (sv.compare("text") == 0) {

            return output_format::PLAIN_TEXT;
        } else if (sv.compare("json") == 0) {

            return output_format::JSON;
        } else if (sv.compare("obj") == 0) {

            return output_format::WAVEFRONT_OBJECT_FILE;
        } else if (sv.compare("object") == 0) {

            return output_format::WAVEFRONT_OBJECT_FILE;
        } else if (sv.compare("png") == 0) {

            return output_format::PNG;
        } else if (sv.compare("jpeg") == 0) {

            return output_format::JPEG;
        } else if (sv.compare("jpg") == 0) {

            return output_format::JPEG;
        } else if (sv.compare("stdout") == 0) {

            return output_format::STDOUT;
        } else {

            throw std::invalid_argument("Invalid output_format: " + std::string{ sv });
        }
    };

    /// @brief Enum class for maze types by the generating algorithm
    enum class algo : unsigned int {
        BINARY_TREE = 0,
        SIDEWINDER = 1,
        DFS = 2,
        TOTAL = 3
    };

    /// @brief Convert the algo enum to a string
    /// @param a
    /// @return string
    inline std::string to_string_from_algo(algo a) {
        switch (a) {
        case algo::BINARY_TREE:
            return "binary_tree";
        case algo::SIDEWINDER:
            return "sidewinder";
        case algo::DFS:
            return "dfs";
        default:
            throw std::invalid_argument("Invalid algo: " + std::to_string(static_cast<unsigned int>(a)));
        }
    };

    /// @brief Convert a string to an algo enum
    /// @param a
    /// @return algo
    inline algo to_algo_from_string(const std::string& a) {
        if (a.compare("binary_tree") == 0) {
            return algo::BINARY_TREE;
        } else if (a.compare("sidewinder") == 0) {
            return algo::SIDEWINDER;
        } else if (a.compare("dfs") == 0) {
            return algo::DFS;
        } else {
            throw std::invalid_argument("Invalid algo: " + a);
        }
    };

    /// @brief Directional neighbors for grid topology
    enum class Direction : std::uint8_t {
        NORTH = 0,
        SOUTH = 1,
        EAST = 2,
        WEST = 3,
        COUNT
    };
} // namespace

#endif // ENUMS
