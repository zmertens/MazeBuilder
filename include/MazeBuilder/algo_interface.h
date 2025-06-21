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
/// @brief Interface for the maze generation algorithms
/// @details This interface provides a method for generating a maze
/// @details Uses a strategy design pattern
class algo_interface {
public:
    /// @brief Interface method that algorithms implement to generate a maze
    /// @param g 
    /// @param rng
    /// @return 
    virtual bool run(std::unique_ptr<grid_interface> const& g, randomizer& rng) const noexcept = 0;
};
}
#endif // ALGO_INTERFACE_H
