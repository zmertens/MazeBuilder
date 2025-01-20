#ifndef HASH_H
#define HASH_H

#include <functional>

namespace mazes {

/// @brief Useful to store a block's 2D positioning in a grid / chunk-based world
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        auto hash1 = std::hash<T1>{}(std::get<0>(p));
        auto hash2 = std::hash<T1>{}(std::get<1>(p));
        return hash1 ^ hash2;
    }
}; // pair_hash
} // namespace

#endif // HASH_H
