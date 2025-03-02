#ifndef STRINGZ_H
#define STRINGZ_H

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <tuple>

namespace mazes {

class maze;

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
    static void objectify(const std::unique_ptr<maze>& m,
        std::vector<std::tuple<int, int, int, int>>& vertices,
        std::vector<std::vector<std::uint32_t>>& faces) noexcept;

    /// @brief Convert a maze into a string representation
    /// @param m the maze to convert
    /// @return the string representation
    static std::string stringify(const std::unique_ptr<maze>& m) noexcept;

}; // class
} // namespace

#endif // STRINGZ_H
