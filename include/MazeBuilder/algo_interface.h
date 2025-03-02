#ifndef ALGO_INTERFACE_H
#define ALGO_INTERFACE_H

#include <memory>
#include <functional>
#include <random>

/// @brief Namespace for the maze builder
namespace mazes {

class grid_interface;

/// @file algo_interface.h

/// @class algo_interface
/// @brief Interface for the maze generation algorithms
/// @details This interface provides a method for generating a maze
class algo_interface {
public:
    /// @brief Interface method that algorithms implement to generate a maze
    /// @param g 
    /// @param get_int 
    /// @param rng 
    /// @return 
    virtual bool run(const std::unique_ptr<grid_interface>& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept = 0;
};
}
#endif // ALGO_INTERFACE_H
