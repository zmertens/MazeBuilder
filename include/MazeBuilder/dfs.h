#ifndef DFS_H
#define DFS_H

#include <MazeBuilder/algo_interface.h>

#include <functional>
#include <memory>
#include <random>

namespace mazes {

class grid_interface;

/// @file dfs.h
/// @class dfs
/// @brief Depth-first search algorithm for generating mazes
/// @details Also-known-as the recursive backtracker algorithm
/// @details This algorithm is used to generate mazes by visiting each cell in the grid and carving a path to the next cell
class dfs : public algo_interface {
    public:
    /// @brief Run the depth-first search algorithm
    /// @param g
    /// @param get_int
    /// @param rng
    /// @return success or failure
    virtual bool run(const std::unique_ptr<grid_interface>& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept override;
};
}

#endif // DFS_H
