#ifndef STRINGIFY_H
#define STRINGIFY_H

#include <MazeBuilder/algo_interface.h>

/// @file stringify.h
/// @namespace mazes
namespace mazes
{
    /// @brief Stringify algorithm for maze representation
    class stringify : public algo_interface
    {
    public:
        /// @brief Run the stringify algorithm
        /// @param g The grid to stringify
        /// @param rng The randomizer to use
        /// @return True if successful, false otherwise
        virtual bool run(grid_interface *g, randomizer &rng) const noexcept override;
    };
}

#endif // STRINGIFY_H
