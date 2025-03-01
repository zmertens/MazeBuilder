#ifndef ENUMS
#define ENUMS

#include <string>
#include <stdexcept>

namespace mazes {

    /// @brief Character representations of walls and barriers in the maze
    static constexpr auto BARRIER1 = '|';
    static constexpr auto BARRIER2 = '-';
    static constexpr auto CORNER = '+';

    /// @brief Enum class for output types
    enum class output : unsigned int {
        PLAIN_TEXT = 0,
        JSON = 1,
        WAVEFRONT_OBJECT_FILE = 2,
        PNG = 3,
        JPEG = 4,
        STDOUT = 5,
        TOTAL = 6
    };

    /// @brief Convert an output enum to a string
    /// @param o
    /// @return 
    static std::string to_string_from_outputs(output o) {
        switch (o) {
        case output::PLAIN_TEXT:
            return "txt";
        case output::JSON:
            return "json";
        case output::WAVEFRONT_OBJECT_FILE:
            return "obj";
        case output::PNG:
            return "png";
        case output::JPEG:
            return "jpeg";
        case output::STDOUT:
            return "stdout";
        default:
            throw std::invalid_argument("Invalid output: " + std::to_string(static_cast<unsigned int>(o)));
        }
    };

    /// @brief Convert a string to an output enum
    /// @param o
    /// @return
    static output to_output_from_string(const std::string& o) {
        if (o.compare("txt") == 0) {
            return output::PLAIN_TEXT;
        } else if (o.compare("json") == 0) {
            return output::JSON;
        } else if (o.compare("obj") == 0) {
            return output::WAVEFRONT_OBJECT_FILE;
        } else if (o.compare("png") == 0) {
            return output::PNG;
        } else if (o.compare("jpeg") == 0) {
            return output::JPEG;
        } else if (o.compare("stdout") == 0) {
            return output::STDOUT;
        } else {
            throw std::invalid_argument("Invalid output: " + o);
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
    static std::string to_string_from_algo(algo a) {
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
    static algo to_algo_from_string(const std::string& a) {
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
} // namespace

#endif // ENUMS
