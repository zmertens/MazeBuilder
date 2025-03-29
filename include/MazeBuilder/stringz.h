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

    /// @brief Compute the 3D geometries of a maze
    /// @param labyrinth the collection of mazes
    /// @param sv 
    static void objectify(lab& labyrinth,
        std::string_view sv = std::string_view{}) noexcept;

    /// @brief Converts a string representation of an image to a pixel array.
    /// @param s 
    /// @param pixels vector to store the resulting pixel data
    /// @param width integer reference to store the width of the image
    /// @param height integer reference to store the height of the image
    static void to_pixels(const std::string& s, std::vector<std::uint8_t>& pixels, int& width, int& height,
        int stride = 4) noexcept;

    /// @brief Convert a maze into a string representation
    /// @param m the maze to convert
    /// @return the string representation
    static std::string stringify(const std::unique_ptr<maze>& m) noexcept;

}; // class
} // namespace

#endif // STRINGZ_H
