#include <MazeBuilder/string_utils.h>

#include <cctype>
#include <cstdint>
#include <sstream>

#include <fmt/format.h>

using namespace mazes;

std::string_view string_utils::concat(const std::string& a, const std::string& b) noexcept
{
    return fmt::format("{}{}", a, b);
}

bool string_utils::contains(const std::string& str, const std::string& substr) noexcept
{
    return str.find(substr) != std::string::npos;
}

bool string_utils::ends_with(const std::string& str, const std::string& suffix) noexcept
{
    return std::string_view{str}.substr(str.size() - suffix.size()) == suffix;
}

