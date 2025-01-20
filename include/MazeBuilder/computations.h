#ifndef COMPUTATIONS_H
#define COMPUTATIONS_H

#include <unordered_map>
#include <memory>

namespace mazes {

class maze;
using maze_ptr = std::unique_ptr<maze>;
class computations {
public:

    /// @brief Compute the 3D geometries and store them in the maze
    static void compute_geometry(const maze_ptr& m) noexcept;


}; // class
} // namespace

#endif // COMPUTATIONS_H