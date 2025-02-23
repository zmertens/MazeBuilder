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
    enum class outputs : unsigned int {
        PLAIN_TEXT = 0,
        JSON = 1,
        WAVEFRONT_OBJECT_FILE = 2,
        PNG = 3,
        JPEG = 4,
        STDOUT = 5,
        TOTAL = 6
    };

    /// @brief Convert an output enum to a string
    /// @param output 
    /// @return 
    static std::string to_string_from_outputs(outputs output) {
        switch (output) {
        case outputs::PLAIN_TEXT:
            return "txt";
        case outputs::JSON:
            return "json";
        case outputs::WAVEFRONT_OBJECT_FILE:
            return "obj";
        case outputs::PNG:
            return "png";
        case outputs::JPEG:
            return "jpeg";
        case outputs::STDOUT:
            return "stdout";
        default:
            throw std::invalid_argument("Invalid output: " + std::to_string(static_cast<unsigned int>(output)));
        }
    };

    /// @brief Convert a string to an output enum
    /// @param output
    /// @return
    static outputs to_output_from_string(const std::string& output) {
        if (output.compare("txt") == 0) {
            return outputs::PLAIN_TEXT;
        } else if (output.compare("json") == 0) {
            return outputs::JSON;
        } else if (output.compare("obj") == 0) {
            return outputs::WAVEFRONT_OBJECT_FILE;
        } else if (output.compare("png") == 0) {
            return outputs::PNG;
        } else if (output.compare("jpeg") == 0) {
            return outputs::JPEG;
        } else if (output.compare("stdout") == 0) {
            return outputs::STDOUT;
        } else {
            throw std::invalid_argument("Invalid output: " + output);
        }
    };

    /// @brief Enum class for maze types by the generating algorithm
    enum class algos : unsigned int {
        BINARY_TREE = 0,
        SIDEWINDER = 1,
        DFS = 2,
        TOTAL = 3
    };

    /// @brief Convert the algo enum to a string
    static std::string to_string_from_algo(algos algo) {
        switch (algo) {
        case algos::BINARY_TREE:
            return "binary_tree";
        case algos::SIDEWINDER:
            return "sidewinder";
        case algos::DFS:
            return "dfs";
        default:
            throw std::invalid_argument("Invalid algo: " + std::to_string(static_cast<unsigned int>(algo)));
        }
    };

    /// @brief Convert a string to an algo enum
    /// @param algo 
    /// @return 
    static algos to_algo_from_string(const std::string& algo) {
        if (algo.compare("binary_tree") == 0) {
            return algos::BINARY_TREE;
        } else if (algo.compare("sidewinder") == 0) {
            return algos::SIDEWINDER;
        } else if (algo.compare("dfs") == 0) {
            return algos::DFS;
        } else {
            throw std::invalid_argument("Invalid algo: " + algo);
        }
    };
} // namespace

#endif // ENUMS
