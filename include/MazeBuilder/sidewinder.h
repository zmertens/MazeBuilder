#ifndef SIDEWINDER_H
#define SIDEWINDER_H

#include <MazeBuilder/algo_interface.h>

namespace mazes {

class grid_interface;
class randomizer;

/// @file sidewinder.h
/// @class sidewinder
/// @brief Sidewinder algorithm for generating mazes
class sidewinder : public algo_interface {

public:

    /// @brief Implement the sidewinder maze generation algorithm
    /// @param g 
    /// @param rng
    /// @return success or failure
    bool run(grid_interface* g, randomizer& rng) const noexcept override;
};

}

#endif // SIDEWINDER_H
