#ifndef FACTORY_INTERFACE_H
#define FACTORY_INTERFACE_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

/// @brief Namespace for the maze builder
namespace mazes {

class configurator;
class grid_interface;

/// @file factory_interface.h
/// @class factory_interface
/// @brief Modern factory interface using registration pattern
/// @details Uses function objects for flexible grid creation with registration capabilities
class factory_interface {
    
public:

    /// @brief Type alias for grid creation function
    using grid_creator_t = std::function<std::unique_ptr<grid_interface>(const configurator&)>;

    /// @brief Register a grid creator function with a unique identifier
    /// @param key Unique identifier for the grid type
    /// @param creator Function object that creates the grid
    /// @return True if registration was successful, false if key already exists
    virtual bool register_creator(const std::string& key, grid_creator_t creator) = 0;

    /// @brief Unregister a grid creator function
    /// @param key Unique identifier for the grid type to remove
    /// @return True if unregistration was successful, false if key was not found
    virtual bool unregister_creator(const std::string& key) = 0;

    /// @brief Check if a creator is registered for the given key
    /// @param key Unique identifier to check
    /// @return True if a creator is registered for this key
    virtual bool is_registered(const std::string& key) const = 0;

    /// @brief Create a grid using a registered creator
    /// @param key Unique identifier for the grid type
    /// @param config Configuration parameters
    /// @return Unique pointer to the created grid, nullptr if key not found or creation failed
    virtual std::unique_ptr<grid_interface> create(const std::string& key, const configurator& config) const = 0;

    /// @brief Create a grid pointer using default logic (for backwards compatibility)
    /// @param config Configuration parameters
    /// @return pointer to a grid product
    virtual std::unique_ptr<grid_interface> create(const configurator& config) const = 0;

    /// @brief Get all registered creator keys
    /// @return Vector of registered keys
    virtual std::vector<std::string> get_registered_keys() const = 0;

    /// @brief Clear all registered creators
    virtual void clear() = 0;

    virtual ~factory_interface() = default;
};

} // namespace mazes

#endif // FACTORY_INTERFACE_H
