#ifndef STRINGZ_H
#define STRINGZ_H

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <tuple>

namespace mazes {

class maze;
class grid_interface;

class stringz {
public:

    /// @brief Compute the 3D geometries of a maze
    /// @param m the maze to convert
    /// @param vertices the vertices of the maze
    /// @param faces the faces of the maze
    /// @param offset_x the x offset where the maze should be placed
    /// @param offset_z the z offset where the maze should be placed
    static void objectify(const std::unique_ptr<maze>& m,
        std::vector<std::tuple<int, int, int, int>>& vertices,
        std::vector<std::vector<std::uint32_t>>& faces,
        int offset_x = 0, int offset_z = 0) noexcept;

    /// @brief Convert a maze into a string representation
    /// @param m the maze to convert
    /// @return the string representation
    static std::string stringify(const std::unique_ptr<maze>& m) noexcept;

}; // class
} // namespace

#endif // STRINGZ_H
