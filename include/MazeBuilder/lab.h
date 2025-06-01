#ifndef LAB_H
#define LAB_H

#include <optional>
#include <utility>
#include <tuple>
#include <unordered_map>

#include <MazeBuilder/hash_funcs.h>

/// @brief Namespace for the maze builder
namespace mazes {

class cell;
class maze;

/// @file lab.h

/// @class lab
/// @brief Holds mazes and provides search functions
/// @details Provides fast lookups of maze data
class lab {
public:
    explicit lab();

    // Destructor
    ~lab();

    // Copy constructor
    lab(const lab& other);

    // Copy assignment operator
    lab& operator=(const lab& other);

    // Move constructor
    lab(lab&& other) noexcept = default;

    // Move assignment operator
    lab& operator=(lab&& other) noexcept = default;

    std::optional<std::tuple<int, int, int, int>> find(int p, int q) const noexcept;
    std::optional<std::tuple<int, int, int, int>> find(int p, int q, int r) const noexcept;

    void insert(int x, int y, int z, int w) noexcept;

    bool empty() const noexcept;

    int get_levels() const noexcept;
    void set_levels(int levels) noexcept;

    int get_random_block_id() const noexcept;

    /// @brief Links two cell objects, optionally in both directions.
    /// @param c1 A shared pointer to the first cell object.
    /// @param c2 A shared pointer to the second cell object.
    /// @param bidi A boolean flag indicating if the link should be bidirectional. Defaults to true.
    static void link(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, bool bidi = true) noexcept;

    /// @brief Unlinks two cell objects, optionally in both directions.
    /// @param c1 A shared pointer to the first cell object.
    /// @param c2 A shared pointer to the second cell object.
    /// @param bidi A boolean flag indicating if the unlink should be bidirectional. Defaults to true.
    static void unlink(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, bool bidi = true) noexcept;
private:

    using pqmap = std::unordered_map<std::pair<int, int>, std::tuple<int, int, int, int>, pair_hash>;
    pqmap m_p_q;

    int levels;
};
}

#endif // LAB_H
