#ifndef HASH_H
#define HASH_H

#include <utility>

namespace mazes {

/// @file hash.h

/// @class pair_hash
/// @brief Hashing function to store a block's (x, z) position
struct pair_hash {

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
}; // pair_hash
} // namespace

#endif // HASH_H
