#ifndef BINARY_TREE_H
#define BINARY_TREE_H

#include <MazeBuilder/algo_interface.h>

namespace mazes
{

    class grid_interface;
    class randomizer;

    /// @file binary_tree.h
    /// @class binary_tree
    /// @brief Binary tree algorithm for generating mazes
    class binary_tree : public algo_interface
    {

    public:
        /// @brief Run the binary tree algorithm
        /// @param g
        /// @param rng
        /// @return success or failure
        bool run(grid_interface *g, randomizer &rng) const noexcept override;

    private:
    };
}
#endif // BINARY_TREE_H
