#ifndef DFS_H
#define DFS_H

#include <MazeBuilder/algo_interface.h>

#include <functional>
#include <memory>
#include <random>
#include <vector>

namespace mazes {

class grid_interface;
class randomizer;
class cell;

/// @file dfs.h
/// @class dfs
/// @brief Depth-first search algorithm for generating mazes
/// @details Also-known-as the recursive backtracker algorithm
/// @details This algorithm is used to generate mazes by visiting each cell in the grid and carving a path to the next cell
class dfs : public algo_interface {
public:
    /// @brief Run the depth-first search algorithm
    /// @param g
    /// @param rng
    /// @return success or failure
    virtual bool run(const std::unique_ptr<grid_interface>& g, randomizer& rng) const noexcept override;

private:
    std::vector<std::shared_ptr<cell>> get_unvisited_neighbors(std::shared_ptr<cell> const& c) const noexcept;
};
}

#endif // DFS_H
