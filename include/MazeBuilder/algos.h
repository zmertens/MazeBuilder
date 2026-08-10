#ifndef ALGOS_H
#define ALGOS_H

#include <algorithm>
#include <array>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

/// @namespace mazes
/// @file algos.h
namespace mazes
{
    /// @brief Enumeration of maze generation algorithms
    enum class algo : unsigned int
    {
        BINARY_TREE = 0,
        SIDEWINDER = 1,
        DFS = 2,
        TOTAL = 3
    };

    /// @brief Array of string_view labels for the algo enum, in lowercase
    constexpr std::array<std::string_view, static_cast<size_t>(algo::TOTAL)> ALGOS_LABELS_LOWERCASE = {
        "binary_tree",
        "sidewinder",
        "dfs"
    };

    /// @brief Convert the algo enum to a string_view
    /// @param a
    /// @return
    inline std::string_view to_sv_from_algo(algo a)
    {
        if (a == algo::TOTAL)
        {
            throw std::invalid_argument("Invalid algo: " + std::to_string(static_cast<unsigned int>(a)));
        }
        return ALGOS_LABELS_LOWERCASE.at(static_cast<size_t>(a));
    }

    /// @brief Convert a string_view to an algo enum
    /// @param a
    /// @return algo
    inline algo to_algo_from_sv(std::string_view a)
    {
        if (const auto it = std::ranges::find(ALGOS_LABELS_LOWERCASE, a); it != ALGOS_LABELS_LOWERCASE.end())
        {
            return static_cast<algo>(std::distance(ALGOS_LABELS_LOWERCASE.begin(), it));
        }
        throw std::invalid_argument("Invalid algo: " + std::string{a});
    }
} // namespace mazes

#endif // ALGOS_H
