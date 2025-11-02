#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include <cassert>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

class Texture;

template <typename T>
class HasLoadFromFile {
public:
	static constexpr bool value = requires(T t, std::string_view path) {
		{ t.loadFromFile(path) } -> std::convertible_to<bool>;
	};
};

template <typename T>
class HasLoadFromStr {
public:
	static constexpr bool value = requires(T t, std::string_view str, int param2) {
		{ t.loadFromStr(str, param2) } -> std::convertible_to<bool>;
	};
};

template <typename Resource, typename Identifier>
class ResourceManager
{
public:
    void load(Identifier id, const std::string& filename);

    template <typename Parameter>
    void load(Identifier id, const std::string& filename, const Parameter& secondParam);

    Resource& get(Identifier id);
    const Resource& get(Identifier id) const;


private:
    void insertResource(Identifier id, std::unique_ptr<Resource> resource);


private:
    std::map<Identifier, std::unique_ptr<Resource>> mResourceMap;
};


template <typename Resource, typename Identifier>
void ResourceManager<Resource, Identifier>::load(Identifier id, const std::string& filename)
{
	if constexpr (!HasLoadFromFile<Resource>::value) {

		static_assert(HasLoadFromFile<Resource>::value, "Resource type does not support loadFromFile method.");
	}

	// Create and load resource
	auto resource = std::make_unique<Resource>();

	if (!resource->loadFromFile(filename)) {

		throw std::runtime_error("ResourceManager::load - Failed to load " + filename);
	}

	// If loading successful, insert resource to map
	insertResource(id, std::move(resource));
}

template <typename Resource, typename Identifier>
template <typename Parameter>
void ResourceManager<Resource, Identifier>::load(Identifier id, const std::string& str, const Parameter& secondParam)
{
	if constexpr (!HasLoadFromStr<Resource>::value) {

		static_assert(HasLoadFromStr<Resource>::value, "Resource type does not support loadFromStr method.");
	}

	// Create and load resource
	auto resource = std::make_unique<Resource>();
	if (!resource->loadFromStr(str, secondParam)) {

		throw std::runtime_error("ResourceManager::load - Failed to load " + str);
	}

	// If loading successful, insert resource to map
	insertResource(id, std::move(resource));
}

template <typename Resource, typename Identifier>
Resource& ResourceManager<Resource, Identifier>::get(Identifier id)
{
	auto found = mResourceMap.find(id);
	assert(found != mResourceMap.cend());

	return *found->second;
}

template <typename Resource, typename Identifier>
const Resource& ResourceManager<Resource, Identifier>::get(Identifier id) const
{
	auto found = mResourceMap.find(id);
	assert(found != mResourceMap.cend());

	return *found->second;
}

template <typename Resource, typename Identifier>
void ResourceManager<Resource, Identifier>::insertResource(Identifier id, std::unique_ptr<Resource> resource) 
{
	// Insert and check success
	auto inserted = mResourceMap.insert(std::make_pair(id, std::move(resource)));
	assert(inserted.second);
}

#endif // RESOURCE_MANAGER_HPP

