#include <MazeBuilder/string_utils.h>

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <sstream>

#include <fmt/format.h>

#include <MazeBuilder/output_formats.h>

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

// pos = 1
std::string string_utils::f_ext_str(std::string_view filename, std::size_t pos) noexcept
{
    const auto str{std::filesystem::path(filename).extension().string()};
    if (pos < str.size())
    {
        return str.substr(pos);
    }
    return {};
}
