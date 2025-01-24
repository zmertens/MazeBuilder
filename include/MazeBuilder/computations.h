#ifndef COMPUTATIONS_H
#define COMPUTATIONS_H

#include <unordered_map>
#include <memory>

namespace mazes {

class maze;
using maze_ptr = std::unique_ptr<maze>;
class grid_interface;
using grid_ptr = std::unique_ptr<grid_interface>;
class computations {
public:

    /// @brief Compute the 3D geometries and store them in the maze
    static void compute_geometry(const maze_ptr& m, grid_ptr g = {}) noexcept;


}; // class
} // namespace

#endif // COMPUTATIONS_H
