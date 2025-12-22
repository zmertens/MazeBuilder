#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

#include <dearimgui/imgui.h>

struct SDL_Window;

template <typename Resource, typename Identifier>
class resource_manager
{
public:
    void load(SDL_Window* window, Identifier id, std::string_view filename);

    // Load resource from raw memory data
    void load(Identifier id, unsigned int w, unsigned int h, const std::uint8_t* data, std::uint32_t channel_offset = 0);

    void load(Identifier id, std::string_view filename, std::uint32_t channel_offset = 0);

    void load(Identifier id, std::string_view v, std::string_view f);

    template <typename Parameter1, typename Parameter2, typename PixelSize = float>
    void load(Identifier id, const Parameter1& param1, const Parameter2& param2, const PixelSize& pixelSize);

    Resource& get(Identifier id);
    const Resource& get(Identifier id) const;

    void clear() noexcept
    {
        m_resources_map.clear();
    }

    [[nodiscard]] bool isEmpty() const noexcept { return m_resources_map.empty(); }

private:
    void insert_resource(Identifier id, std::unique_ptr<Resource> resource);
    std::map<Identifier, std::unique_ptr<Resource>> m_resources_map;
};

template <typename Resource, typename Identifier>
void resource_manager<Resource, Identifier>::load(SDL_Window* window, Identifier id, std::string_view filename)
{
    // Create and load resource
    auto resource = std::make_unique<Resource>();

    if (!resource->load_bmp_icon(window, filename))
    {
        throw std::runtime_error("resource_manager::load - Failed to load " + std::string(filename));
    }

    // If loading successful, insert resource to map
    insert_resource(id, std::move(resource));
}

template <typename Resource, typename Identifier>
void resource_manager<Resource, Identifier>::load(Identifier id, unsigned int w, unsigned int h,
    const std::uint8_t* data, std::uint32_t channel_offset)
{
    // Create and load resource
    auto resource = std::make_unique<Resource>();

    if (!resource->load_from_memory(data, w, h, channel_offset))
    {
        throw std::runtime_error("resource_manager::load - Failed to load data to memory");
    }

    // If loading successful, insert resource to map
    insert_resource(id, std::move(resource));
}

template <typename Resource, typename Identifier>
void resource_manager<Resource, Identifier>::load(Identifier id, std::string_view filename, std::uint32_t channel_offset)
{
    // Create and load resource
    auto resource = std::make_unique<Resource>();

    if (!resource->load_from_file(filename, channel_offset))
    {
        throw std::runtime_error("resource_manager::load - Failed to load " + std::string(filename));
    }

    // If loading successful, insert resource to map
    insert_resource(id, std::move(resource));
}

template <typename Resource, typename Identifier>
void resource_manager<Resource, Identifier>::load(Identifier id, std::string_view v, std::string_view f)
{
    // Create and load resource
    auto resource = std::make_unique<Resource>();

    if (!resource->load_program(v, f))
    {
        throw std::runtime_error("resource_manager::load - Failed to load " + std::string(v) + " and " + std::string(f));
    }

    // If loading successful, insert resource to map
    insert_resource(id, std::move(resource));
}

template <typename Resource, typename Identifier>
template <typename Parameter1, typename Parameter2, typename PixelSize>
void resource_manager<Resource, Identifier>::load(Identifier id, const Parameter1& param1, const Parameter2& param2, const PixelSize& pixelSize)
{
    auto resource = std::make_unique<Resource>();
    if (!resource->loadFromMemoryCompressedTTF(param1, param2, pixelSize))
    {
        throw std::runtime_error("resource_manager::load - Failed to load font from memory");
    }

    insert_resource(id, std::move(resource));
}

template <typename Resource, typename Identifier>
Resource& resource_manager<Resource, Identifier>::get(Identifier id)
{
    auto found = m_resources_map.find(id);
    assert(found != m_resources_map.cend());

    return *found->second;
}

template <typename Resource, typename Identifier>
const Resource& resource_manager<Resource, Identifier>::get(Identifier id) const
{
    auto found = m_resources_map.find(id);
    assert(found != m_resources_map.cend());

    return *found->second;
}

template <typename Resource, typename Identifier>
void resource_manager<Resource, Identifier>::insert_resource(Identifier id, std::unique_ptr<Resource> resource)
{
    // Insert and check success
    auto inserted = m_resources_map.insert(std::make_pair(id, std::move(resource)));
    assert(inserted.second);
}

#endif // RESOURCE_MANAGER_HPP
