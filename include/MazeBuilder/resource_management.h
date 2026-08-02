#ifndef RESOURCE_MANAGEMENT_H
#define RESOURCE_MANAGEMENT_H

#include <memory>
#include <string_view>
#include <stdexcept>
#include <unordered_map>
#include <utility>

/// @file resource_management.h
/// @namespace mazes
namespace mazes
{
    /// @brief Template class for managing resources
    /// @tparam Resource The type of resource to manage
    /// @tparam Identifier The type used to identify resources
    template <typename Resource, typename Identifier>
    class resource_management final
    {
    public:
        /// @brief Loads a resource of a specific type
        /// @tparam Concrete The concrete type of the resource
        /// @tparam ...Args The types of arguments to pass to the resource constructor
        /// @param id The identifier for the resource
        /// @param args The arguments to pass to the resource constructor
        /// @param ...args The arguments to pass to the resource constructor
        template <typename Concrete, typename... Args>
        void load(Identifier id, Args&&... args)
        {
            auto resource = std::make_unique<Concrete>(std::forward<Args>(args)...);
            if (!resource)
            {
                throw std::runtime_error("Failed to load resource");
            }
            insert_resource(id, std::move(resource));
        }

        /// @brief Loads a resource from a string
        /// @param id
        /// @param txt
        void load(Identifier id, std::string_view txt);

        /// @brief Retrieves a resource by its identifier
        /// @param id
        /// @return
        Resource& get(Identifier id);

        /// @brief Retrieves a const reference to a resource by its identifier
        /// @param id
        /// @return
        const Resource& get(Identifier id) const;

        /// @brief Clears all resources from the manager
        void clear() noexcept
        {
            m_resources_map.clear();
        }

        /// @brief Checks if the resource manager is empty
        [[nodiscard]] bool is_empty() const noexcept { return m_resources_map.empty(); }

    private:
        /// @brief Inserts a resource into the manager
        /// @param id The identifier for the resource
        /// @param resource The resource to be inserted
        void insert_resource(Identifier id, std::unique_ptr<Resource> resource);

        std::unordered_map<Identifier, std::unique_ptr<Resource>> m_resources_map;
    };

    template <typename Resource, typename Identifier>
    void resource_management<Resource, Identifier>::load(Identifier id, std::string_view txt)
    {
        // Create and load resource
        auto resource = std::make_unique<Resource>();

        if (!resource->set(txt))
        {
            throw std::runtime_error("Failed to load resource");
        }

        // If loading successful, insert resource to map
        insert_resource(id, std::move(resource));
    }

    /// @brief Non-const version of get() to retrieve a resource by its identifier
    /// @tparam Resource
    /// @tparam Identifier
    /// @param id
    /// @return
    template <typename Resource, typename Identifier>
    Resource& resource_management<Resource, Identifier>::get(Identifier id)
    {
        auto found = m_resources_map.find(id);
        if (found == m_resources_map.cend())
        {
            throw std::out_of_range("Key not found");
        }

        return *found->second;
    }

    /// @brief Const version of get() to retrieve a resource by its identifier
    /// @tparam Resource
    /// @tparam Identifier
    /// @param id
    /// @return
    template <typename Resource, typename Identifier>
    const Resource& resource_management<Resource, Identifier>::get(Identifier id) const
    {
        auto found = m_resources_map.find(id);
        if (found == m_resources_map.cend())
        {
            throw std::out_of_range("Key not found");
        }

        return *found->second;
    }

    /// @brief Inserts a resource into the manager
    /// @tparam Resource
    /// @tparam Identifier
    /// @param id
    /// @param resource
    template <typename Resource, typename Identifier>
    void resource_management<Resource, Identifier>::insert_resource(Identifier id, std::unique_ptr<Resource> resource)
    {
        m_resources_map.insert_or_assign(id, std::move(resource));
    }
} // namespace mazes

#endif // RESOURCE_MANAGEMENT_H
