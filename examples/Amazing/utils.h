#ifndef UTILS_H
#define UTILS_H

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include <MazeBuilder/async_logger.h>
#include <MazeBuilder/singleton_base.h>

namespace utils
{
    class async_loader : public mazes::singleton_base<async_loader>
    {
        friend class mazes::singleton_base<async_loader>;

    public:
        static const std::filesystem::path &get_resource_path() noexcept;
        static void set_resource_path(const std::filesystem::path &path) noexcept;

        static std::map<std::string, std::filesystem::path> load_resource_map(
            const std::filesystem::path &resource_json = get_resource_path());

        static std::optional<std::filesystem::path> resolve_resource_path(
            std::string_view key,
            const std::filesystem::path &resource_json = get_resource_path());

        static void load(const std::function<void(std::map<std::string, std::filesystem::path> &)> &initializer) noexcept;

    private:
        static std::filesystem::path resource_path;

        static mazes::async_logger logger;
    };
}

#endif // UTILS_H