#ifndef SIDEWINDER_HPP
#define SIDEWINDER_HPP

#include <MazeBuilder/algo_interface.h>

#include <functional>
#include <memory>

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

#endif // SIDEWINDER_HPP
