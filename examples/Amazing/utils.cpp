#include "utils.h"

#include <MazeBuilder/json_helper.h>

#include <fstream>
#include <stdexcept>

using namespace utils;

std::filesystem::path async_loader::resource_path = {};
mazes::async_logger async_loader::logger{};

const std::filesystem::path &async_loader::get_resource_path() noexcept
{
    return resource_path;
}

void async_loader::set_resource_path(const std::filesystem::path &path) noexcept
{
    resource_path = path;
}

std::map<std::string, std::filesystem::path> async_loader::load_resource_map(
    const std::filesystem::path &resource_json)
{
    std::map<std::string, std::filesystem::path> resources;

    if (resource_json.empty() || !std::filesystem::exists(resource_json))
    {
        logger.log("Resource map \'" + resource_json.string() + "\' was not found.");
        return resources;
    }

    std::unordered_map<std::string, std::string> raw;
    if (!mazes::json_helper::load(resource_json.string(), raw))
    {
        logger.log("Failed to parse resource map \'" + resource_json.string() + "\'");
        return resources;
    }

    for (const auto &[key, value] : raw)
    {
        const auto resolved = (resource_json.parent_path() / value).lexically_normal();
        resources.emplace(key, resolved);
    }

    return resources;
}

std::optional<std::filesystem::path> async_loader::resolve_resource_path(
    std::string_view key,
    const std::filesystem::path &resource_json)
{
    const auto resources = load_resource_map(resource_json);
    const auto match = resources.find(std::string{key});
    if (match == resources.end())
    {
        return std::nullopt;
    }

    const auto &resolved = match->second;
    if (!resolved.empty() && std::filesystem::exists(resolved))
    {
        return resolved;
    }

    return std::nullopt;
}

void async_loader::load(const std::function<void(std::map<std::string, std::filesystem::path> &)> &initializer) noexcept
{
    try
    {
        if (resource_path.empty())
        {
            logger.log("No resource path set for async loader.");
            return;
        }

        auto &&resource_map = load_resource_map(resource_path);
        logger.log("Loaded " + std::to_string(resource_map.size()) + " resources from \'" + resource_path.string() + "\'");
        initializer(std::ref(resource_map));
    }
    catch (const std::exception &ex)
    {
        logger.log("Exception caught during resource loading: " + std::string(ex.what()));
    }
}
