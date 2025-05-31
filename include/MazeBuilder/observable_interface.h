#ifndef OBSERVABLE_INTERFACE_H
#define OBSERVABLE_INTERFACE_H

namespace mazes {

/// @file observable_interface.h
/// @class observable_interface
/// @brief Interface for observable entities
/// @details This interface provides methods to observer methods
class observable_interface {

public:
    /// @brief Registers an observer function
    /// @param observer A function to be called when the grid is updated
    //virtual void register_observer(std::function<bool(void)> const& observer) noexcept = 0;

    ///// @brief Starts the configuration of the grid
    ///// @param indices A vector of integers representing the configuration indices
    //virtual bool is_observed() noexcept = 0;



}; // observable_interface

} // namespace mazes

#endif // OBSERVABLE_INTERFACE_H
