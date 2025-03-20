#ifndef LAB_H
#define LAB_H

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <optional>
#include <utility>
#include <tuple>
#include <unordered_map>

#include <MazeBuilder/hash_funcs.h>

/// @brief Namespace for the maze builder
namespace mazes {

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

    void append(const std::optional<std::unique_ptr<maze>>& m) noexcept;

    std::optional<std::tuple<int, int, int, int>> find(int x, int z) const noexcept;

    void insert(int x, int y, int z, int w) noexcept;

    bool empty() const noexcept;
private:

    using pqmap = std::unordered_map<std::tuple<int, int, int>, std::tuple<int, int, int, int>, tri_hash>;
    pqmap m_p_q;
    std::vector<std::tuple<int, int, int, int>> m_vertices;
    std::vector<std::vector<std::uint32_t>> m_faces;
};
}

#endif // LAB_H
