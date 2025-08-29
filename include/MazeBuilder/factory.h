#ifndef FACTORY_H
#define FACTORY_H

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

/// @file factory.h
/// @class factory
/// @brief Modern grid factory with registration capabilities
/// @details Provides a way to create grids using registered function objects
/// Thread-safe registration and creation of grid instances
class factory : public factory_interface {

public:

    /// @brief Default constructor
    factory();

    /// @brief Destructor
    ~factory() override = default;

    // Delete copy constructor and assignment for thread safety
    factory(const factory&) = delete;
    factory& operator=(const factory&) = delete;

    // Allow move operations
    factory(factory&&) noexcept = default;
    factory& operator=(factory&&) noexcept = default;

    /// @brief Register a grid creator function with a unique identifier
    /// @param key Unique identifier for the grid type
    /// @param creator Function object that creates the grid
    /// @return True if registration was successful, false if key already exists
    bool register_creator(const std::string& key, grid_creator_t creator) override;

    /// @brief Unregister a grid creator function
    /// @param key Unique identifier for the grid type to remove
    /// @return True if unregistration was successful, false if key was not found
    bool unregister_creator(const std::string& key) override;

    /// @brief Check if a creator is registered for the given key
    /// @param key Unique identifier to check
    /// @return True if a creator is registered for this key
    bool is_registered(const std::string& key) const override;

    /// @brief Create a grid using a registered creator
    /// @param key Unique identifier for the grid type
    /// @param config Configuration parameters
    /// @return Unique pointer to the created grid, nullptr if key not found or creation failed
    std::unique_ptr<grid_interface> create(const std::string& key, const configurator& config) const override;

    /// @brief Create a grid pointer using default logic (for backwards compatibility)
    /// @param config Configuration parameters
    /// @return pointer to a grid product
    std::unique_ptr<grid_interface> create(const configurator& config) const noexcept override;

    /// @brief Get all registered creator keys
    /// @return Vector of registered keys
    std::vector<std::string> get_registered_keys() const override;

    /// @brief Clear all registered creators
    void clear() override;

private:

    /// @brief Thread-safe map of registered creators
    mutable std::mutex m_creators_mutex;
    std::unordered_map<std::string, grid_creator_t> m_creators;

    /// @brief Register default creators for built-in grid types
    void register_default_creators();

    /// @brief Determine grid type key from configuration for backwards compatibility
    /// @param config Configuration parameters
    /// @return String key for the appropriate grid type
    std::string determine_grid_type_from_config(const configurator& config) const;
};

} // namespace mazes

#endif // FACTORY_H
