#ifndef ALGO_INTERFACE_H
#define ALGO_INTERFACE_H

#include <functional>
#include <memory>
#include <random>

/// @brief Namespace for the maze builder
namespace mazes {

class grid_interface;
class randomizer;

/// @file algo_interface.h
/// @class algo_interface
/// @brief Interface for runnable algorithms
/// @details Uses the strategy design pattern
class algo_interface {
    
public:
    /// @brief Interface method that algorithms must implement
    /// @param g 
    /// @param rng
    /// @return 
    virtual bool run(std::unique_ptr<grid_interface> const& g, randomizer& rng) const noexcept = 0;

    virtual ~algo_interface() = default;
};
}
#endif // ALGO_INTERFACE_H
