#ifndef DFS_H
#define DFS_H

#include <MazeBuilder/algo_interface.h>

#include <memory>
#include <vector>

namespace mazes {

class cell;
class grid;
class grid_interface;
class randomizer;

/// @file dfs.h
/// @class dfs
/// @brief Depth-first search algorithm for generating mazes
/// @details This algorithm uses random searches to explore and visit cells and their neighbors
class dfs : public algo_interface {
public:
    /// @brief Run the depth-first search algorithm
    /// @param g
    /// @param rng
    /// @return success or failure
    virtual bool run(std::unique_ptr<grid_interface> const& g, randomizer& rng) const noexcept override;

};

}

#endif // DFS_H
