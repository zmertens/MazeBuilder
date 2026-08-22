#include "resource_paths.hpp"

#include <MazeBuilder/json_helper.h>

#include <stdexcept>
#include <utility>

namespace amazing::game
{
    bool resource_paths::load_from_file(const std::string& filename) noexcept
    {
        std::unordered_map<std::string, std::string> loaded_paths;
        if (!mazes::json_helper::load(filename, loaded_paths))
        {
            return false;
        }

        paths_ = std::move(loaded_paths);
        return true;
    }

    bool resource_paths::contains(const std::string_view key) const noexcept
    {
        return paths_.contains(std::string{ key });
    }

    const std::string& resource_paths::get(const std::string_view key) const
    {
        const auto it = paths_.find(std::string{ key });
        if (it == paths_.end())
        {
            throw std::out_of_range("Missing resource path key: " + std::string{ key });
        }

        return it->second;
    }
}
