#ifndef STRINGZ_H
#define STRINGZ_H

#include <cstdint>
#include <memory>
#include <string_view>
#include <string>
#include <tuple>
#include <vector>

namespace mazes {

class maze;
class lab;

/// @file stringz.h

/// @class stringz
/// @brief String helper class
/// @details This class provides methods to convert mazes into string representations
class stringz {
public:

    /// @brief Compute the 3D geometries of a maze
    /// @param m the maze to convert
    /// @param vertices the vertices of the maze
    /// @param faces the faces of the maze
    /// @param s the string representation of the maze
    static void objectify(const std::unique_ptr<maze>& m,
        std::vector<std::tuple<int, int, int, int>>& vertices,
        std::vector<std::vector<std::uint32_t>>& faces,
        std::string_view sv = std::string_view{}) noexcept;

    static void objectify(lab& labyrinth,
        std::string_view sv = std::string_view{}) noexcept;

    /// @brief Convert a maze into a string representation
    /// @param m the maze to convert
    /// @return the string representation
    static std::string stringify(const std::unique_ptr<maze>& m) noexcept;

}; // class
} // namespace

#endif // STRINGZ_H
