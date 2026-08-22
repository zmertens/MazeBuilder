#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace amazing::game
{
    class resource_paths
    {
    public:
        bool load_from_file(const std::string& filename) noexcept;
        [[nodiscard]] bool contains(std::string_view key) const noexcept;
        [[nodiscard]] const std::string& get(std::string_view key) const;

    private:
        std::unordered_map<std::string, std::string> paths_;
    };
}
