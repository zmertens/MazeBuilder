#ifndef GRID_FACTORY_H
#define GRID_FACTORY_H

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <MazeBuilder/factory_interface.h>

namespace mazes {

class configurator;
class grid_interface;

/// @file grid_factory.h
/// @class grid_factory
/// @brief Modern grid grid_factory with registration capabilities
/// @details Provides a way to create grids using registered function objects
/// Thread-safe registration and creation of grid instances
class grid_factory : public grid_factory_interface {

public:

    /// @brief Register a grid creator function with a unique identifier
    /// @param key Unique identifier for the grid type
    /// @param creator Function object that creates the grid
    /// @return True if registration was successful, false if key already exists
    bool register_creator(const std::string& key, factory_creator_t creator) noexcept override;

    /// @brief Unregister a grid creator function
    /// @param key Unique identifier for the grid type to remove
    /// @return True if unregistration was successful, false if key was not found
    bool unregister_creator(const std::string& key) noexcept override;

    /// @brief Check if a creator is registered for the given key
    /// @param key Unique identifier to check
    /// @return True if a creator is registered for this key
    bool is_registered(const std::string& key) const override;

    /// @brief Create a grid using a registered creator
    /// @param key Unique identifier for the grid type
    /// @param config Configuration parameters
    /// @return Unique pointer to the created grid, nullptr if key not found or creation failed
    std::optional<std::unique_ptr<grid_interface>> create(const std::string& key, const configurator& config) const noexcept override;
    
    /// @brief Get all registered creator keys
    /// @return Vector of registered keys
    std::vector<std::string> get_registered_keys() const override;

    /// @brief Clear all registered creators
    void clear() noexcept override;

private:

    /// @brief Thread-safe map of registered creators
    mutable std::mutex m_creators_mutex;
    std::unordered_map<std::string, factory_creator_t> m_creators;
};

} // namespace mazes

#endif // GRID_FACTORY_H
