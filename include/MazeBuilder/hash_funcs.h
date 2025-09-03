#ifndef HASH_FUNCS_H
#define HASH_FUNCS_H

#include <functional>
#include <memory>
#include <tuple>
#include <utility>

namespace mazes
{

    /// @file hash_funcs.h

    /// @class uni_hash
    /// @brief A hash function object for a single value.
    struct uni_hash
    {
        /// @brief Hash function for a single
        /// @tparam T1
        /// @param p
        /// @return the hash value
        template <class T1>
        std::size_t operator()(const T1 &p) const
        {
            auto hash1 = std::hash<T1>{}(p);
            auto hash2 = std::hash<T1>{}(p) + 0x9e3779b9;
            return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
        }
    };

    /// @class pair_hash
    /// @brief Hashing function to store a block's (x, z) position
    struct pair_hash
    {

        /// @brief Hash function for a pair
        /// @tparam T1
        /// @tparam T2
        /// @param p
        /// @return the hash value
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2> &p) const
        {
            auto hash1 = std::hash<T1>{}(p.first);
            auto hash2 = std::hash<T2>{}(p.second);
            return hash1 ^ hash2;
        }
    }; // pair_hash

    /// @class tri_hash
    /// @brief Hashing function to store a block's (x, y, z) position
    struct tri_hash
    {
        /// @brief Hash function for a tuple
        /// @tparam T1
        /// @tparam T2
        /// @tparam T3
        /// @param p
        /// @return the hash value
        template <class T1, class T2, class T3>
        std::size_t operator()(const std::tuple<T1, T2, T3> &p) const
        {
            auto hash1 = std::hash<T1>{}(std::get<0>(p));
            auto hash2 = std::hash<T2>{}(std::get<1>(p));
            auto hash3 = std::hash<T3>{}(std::get<2>(p));
            return hash1 ^ hash2 ^ hash3;
        }
    };

    /// @class weak_ptr_hash
    /// @brief Hashing function to store a weak_ptr
    struct weak_ptr_hash
    {

        /// @brief Hashing function
        /// @tparam T
        /// @param weak
        /// @return
        template <typename T>
        std::size_t operator()(const std::weak_ptr<T> &weak) const
        {
            if (auto shared = weak.lock())
            {
                return std::hash<std::shared_ptr<T>>{}(shared);
            }

            // Return 0 for expired weak_ptr
            return 0;
        }
    };

} // namespace mazes

#endif // HASH_FUNCS_H
