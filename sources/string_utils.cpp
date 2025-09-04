#include <MazeBuilder/string_utils.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid_interface.h>

#include <algorithm>
#include <cctype>
#include <random>
#include <sstream>
#include <tuple>
#include <unordered_set>
#include <list>

#include <fmt/format.h>

using namespace mazes;

bool string_utils::contains(const std::string &str, const std::string &substr) noexcept
{

    return str.find(substr) != std::string::npos;
}

std::string string_utils::get_file_extension(const std::string &filename) noexcept
{

    auto pos = filename.find_last_of(".");

    if (pos == std::string::npos)
    {

        return "";
    }

    return filename.substr(pos);
}

bool string_utils::ends_with(const std::string &str, const std::string &suffix) noexcept
{

    if (str.length() < suffix.length())
    {

        return false;
    }

    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool string_utils::find(std::string_view sv, char c) noexcept
{

    return sv.find(c) != std::string_view::npos;
}

std::string_view string_utils::find_first_of(const std::string_view &sv, const std::string_view &chars) noexcept
{

    if (sv.empty() || chars.empty())
    {

        return {};
    }

    auto pos = sv.find_first_of(chars);

    if (pos == std::string_view::npos)
    {

        return {};
    }

    return sv.substr(pos, 1);
}

// Template wrapper functions for fmt::format using perfect forwarding and variadic arguments
template <typename... Args>
std::string string_utils::format(std::string_view format_str, const Args &...args) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(args...));
}

// Explicit specializations
template <>
std::string string_utils::format<>(std::string_view format_str) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args());
}

template <>
std::string string_utils::format<int>(std::string_view format_str, const int &arg) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg));
}

template <>
std::string string_utils::format<int, int>(std::string_view format_str, const int &arg1, const int &arg2) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg1, arg2));
}

template <>
std::string string_utils::format<int, float>(std::string_view format_str, const int &arg1, const float &arg2) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg1, arg2));
}

template <>
std::string string_utils::format<int, int, int>(std::string_view format_str, const int &arg, const int &arg2, const int& arg3) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg, arg2, arg3));
}

template <>
std::string string_utils::format<unsigned int, int, int>(std::string_view format_str, const unsigned int &arg, const int &arg2, const int& arg3) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg, arg2, arg3));
}

template <>
std::string string_utils::format<int, int, int, int>(std::string_view format_str, const int &arg, const int &arg2, const int& arg3, const int& arg4) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg, arg2, arg3, arg4));
}

template <>
std::string string_utils::format<int, size_t, size_t, size_t>(std::string_view format_str, const int &arg, const size_t &arg2, const size_t &arg3, const size_t &arg4) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg, arg2, arg3, arg4));
}

template <>
std::string string_utils::format<int, int, float>(std::string_view format_str, const int &arg, const int &arg2, const float &arg3) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg, arg2, arg3));
}

template <>
std::string string_utils::format<float>(std::string_view format_str, const float &arg) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg));
}

template <>
std::string string_utils::format<float, float>(std::string_view format_str, const float &arg1, const float &arg2) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg1, arg2));
}

template <>
std::string string_utils::format<const char *>(std::string_view format_str, const char *const &arg) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg));
}

template <>
std::string string_utils::format<std::string>(std::string_view format_str, const std::string &arg) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg));
}

template <>
std::string string_utils::format<std::string_view>(std::string_view format_str, const std::string_view &arg) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg));
}

template <>
std::string string_utils::format<std::string_view>(std::string_view format_str, const std::string_view &arg1, const std::string_view &arg2) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg1, arg2));
}

template <>
std::string string_utils::format<const char *, const char *>(std::string_view format_str, const char *const &arg1, const char *const &arg2) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg1, arg2));
}

template <>
std::string string_utils::format<size_t>(std::string_view format_str, const size_t &arg) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg));
}

template <>
std::string string_utils::format<std::string, const char *>(std::string_view format_str, const std::string &arg1, const char *const &arg2) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg1, arg2));
}

template <>
std::string string_utils::format<const char *, std::string>(std::string_view format_str, const char *const &arg1, const std::string &arg2) noexcept
{

    return fmt::vformat(format_str, fmt::make_format_args(arg1, arg2));
}
