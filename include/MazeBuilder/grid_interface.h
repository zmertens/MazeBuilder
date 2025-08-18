#ifndef GRID_INTERFACE_H
#define GRID_INTERFACE_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mazes {

class cell;

/// @file grid_interface.h
/// @class grid_interface
/// @brief Interface for the grid class
/// @details The interface provides methods to interact with the grid
/// @details The interface has detailed information about a cell
class grid_interface {

public:
    virtual ~grid_interface() = default;

    /// @brief Get detailed information of a cell in the grid in the form of a string
    /// @param c 
    /// @return 
    virtual std::string contents_of(std::shared_ptr<cell> const& c) const noexcept = 0;

    /// @brief Returns the background color for the specified cell, if available.
    /// @param c A shared pointer to the cell for which to determine the background color.
    /// @return An optional 32-bit unsigned integer representing the background color of the cell
    virtual std::uint32_t background_color_for(std::shared_ptr<cell> const& c) const noexcept = 0;

    /// @brief Get access to grid operations interface
    /// @return A reference to the grid operations interface
    virtual class grid_operations& operations() noexcept = 0;

    /// @brief Get access to const grid operations interface
    /// @return A const reference to the grid operations interface
    virtual const class grid_operations& operations() const noexcept = 0;
}; // grid_interface

} // namespace mazes

#endif // GRID_INTERFACE_H
