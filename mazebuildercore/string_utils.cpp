#include <MazeBuilder/string_utils.h>

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <sstream>

#include <fmt/format.h>

using namespace mazes;

std::string string_utils::concat(const std::string& a, const std::string& b) noexcept
{
    return fmt::format("{}{}", a, b);
}

bool string_utils::contains(const std::string& str, const std::string& substr) noexcept
{
    return str.find(substr) != std::string::npos;
}

bool string_utils::ends_with(const std::string& str, const std::string& suffix) noexcept
{
    return std::string_view{ str }.substr(str.size() - suffix.size()) == suffix;
}

std::string string_utils::file_extension(std::string_view filename) noexcept
{
    const auto& ext = std::filesystem::path{ filename }.extension().string();
    if (ext.empty() || ext == ".")
    {
        return {};
    }
    return ext.substr(1);
}
