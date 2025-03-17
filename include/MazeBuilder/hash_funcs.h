#ifndef HASH_FUNCS_H
#define HASH_FUNCS_H

#include <utility>

namespace mazes {

/// @file hash_funcs.h

/// @class hash_funcs
/// @brief Hashing function to store a block's (x, z) position
struct hash_funcs {

    /// @brief Hash function for a pair
    /// @tparam T1
    /// @tparam T2
    /// @param p
    /// @return the hash value
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        auto hash1 = std::hash<T1>{}(std::get<0>(p));
        auto hash2 = std::hash<T1>{}(std::get<1>(p));
        return hash1 ^ hash2;
    }
}; // hash_funcs
} // namespace

#endif // HASH_FUNCS_H
