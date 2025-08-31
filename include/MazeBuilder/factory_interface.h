#ifndef FACTORY_INTERFACE_H
#define FACTORY_INTERFACE_H

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

/// @brief Namespace for the maze builder
namespace mazes {

class configurator;

/// @file factory_interface.h
/// @class factory_interface
/// @brief Modern factory interface using registration pattern
/// @details Uses function objects for flexible product creation with registration capabilities
/// @tparam InterfaceType The interface type that products must implement
template<typename InterfaceType>
class factory_interface {
    
public:

    /// @brief Type alias for product creation function
    using factory_creator_t = std::function<std::unique_ptr<InterfaceType>(const configurator&)>;

    /// @brief Register a product creator function with a unique identifier
    /// @param key Unique identifier for the product type
    /// @param creator Function object that creates the product
    /// @return True if registration was successful, false if key already exists
    virtual bool register_creator(const std::string& key, factory_creator_t creator) noexcept = 0;

    /// @brief Unregister a product creator function
    /// @param key Unique identifier for the product type to remove
    /// @return True if unregistration was successful, false if key was not found
    virtual bool unregister_creator(const std::string& key) noexcept = 0;

    /// @brief Check if a creator is registered for the given key
    /// @param key Unique identifier to check
    /// @return True if a creator is registered for this key
    virtual bool is_registered(const std::string& key) const = 0;

    /// @brief Create a product using a registered creator
    /// @param key Unique identifier for the product type
    /// @param config Configuration parameters
    /// @return Unique pointer to the created product, nullptr if key not found or creation failed
    virtual std::optional<std::unique_ptr<InterfaceType>> create(const std::string& key, const configurator& config) const noexcept = 0;

    /// @brief Get all registered creator keys
    /// @return Vector of registered keys
    virtual std::vector<std::string> get_registered_keys() const = 0;

    /// @brief Clear all registered creators
    virtual void clear() noexcept = 0;

    virtual ~factory_interface() = default;
};

// Forward declaration for grid_interface
class grid_interface;

/// @brief Type alias for grid factory interface
using grid_factory_interface = factory_interface<grid_interface>;

} // namespace mazes

#endif // FACTORY_INTERFACE_H
