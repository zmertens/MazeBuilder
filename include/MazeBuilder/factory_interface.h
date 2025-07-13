#ifndef FACTORY_INTERFACE_H
#define FACTORY_INTERFACE_H

#include <memory>

/// @brief Namespace for the maze builder
namespace mazes {

class configurator;
class grid_interface;

/// @file factory_interface.h
/// @class factory_interface
/// @details Uses the strategy design pattern
class factory_interface {
    
public:

    /// @brief Create a grid pointer
    /// @param config 
    /// @return pointer to a grid product
    virtual std::unique_ptr<grid_interface> create(configurator const& config) const noexcept = 0;

    virtual ~factory_interface() = default;
};
}
#endif // FACTORY_INTERFACE_H
