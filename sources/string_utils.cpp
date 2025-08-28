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


bool string_utils::contains(const std::string& str, const std::string& substr) noexcept {

    return str.find(substr) != std::string::npos;
}

std::string string_utils::get_file_extension(const std::string& filename) noexcept {
    auto pos = filename.find_last_of(".");
    if (pos == std::string::npos) {
        return "";
    }
    return filename.substr(pos);
}

bool string_utils::ends_with(const std::string& str, const std::string& suffix) noexcept {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

std::string_view string_utils::find_first_of(const std::string_view& s, const std::string_view& chars) noexcept {

    if (s.empty() || chars.empty()) {

        return {};
    }
    
    auto pos = s.find_first_of(chars);
    if (pos == std::string_view::npos) {

        return {};
    }
    
    return s.substr(pos, 1);
}

std::list<std::string> string_utils::split(const std::string& str, char delimiter) noexcept {
    std::list<std::string> result;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        result.push_back(token);
    }
    
    return result;
}

std::list<std::string_view> string_utils::split(const std::string_view& sv, const std::string_view& delim) noexcept {
    std::list<std::string_view> result;
    
    if (sv.empty()) {
        return result;
    }
    
    size_t start = 0;
    size_t pos = sv.find(delim);
    
    while (pos != std::string_view::npos) {
        if (pos > start) {
            result.push_back(sv.substr(start, pos - start));
        }
        start = pos + delim.length();
        pos = sv.find(delim, start);
    }
    
    // Add the last part
    if (start < sv.length()) {
        result.push_back(sv.substr(start));
    } else if (start == sv.length() && !sv.empty()) {
        // Handle the case where the string ends with the delimiter
        result.push_back(std::string_view{});
    }
    
    return result;
}


// Template wrapper functions for fmt::format using perfect forwarding and variadic arguments
template<typename... Args>
static std::string string_utils::format(std::string_view format_str, const Args&... args) noexcept {

    return fmt::vformat(format_str, fmt::make_format_args(args...));
}

// Explicit specializations
template<>
std::string string_utils::format<>(std::string_view format_str) noexcept {

    return fmt::vformat(format_str, fmt::make_format_args());
}

template<>
std::string string_utils::format<int>(std::string_view format_str, const int& arg) noexcept {
    
    return fmt::vformat(format_str, fmt::make_format_args(arg));
}

template<>
std::string string_utils::format<int, int>(std::string_view format_str, const int& arg1, const int& arg2) noexcept {

    return fmt::vformat(format_str, fmt::make_format_args(arg1, arg2));
}

template<>
std::string string_utils::format<int, float>(std::string_view format_str, const int& arg1, const float& arg2) noexcept {

    return fmt::vformat(format_str, fmt::make_format_args(arg1, arg2));
}

template<>
std::string string_utils::format<int, int, float>(std::string_view format_str, const int& arg, const int& arg2, const float& arg3) noexcept {

    return fmt::vformat(format_str, fmt::make_format_args(arg, arg2, arg3));
}

template<>
std::string string_utils::format<float>(std::string_view format_str, const float& arg) noexcept {

    return fmt::vformat(format_str, fmt::make_format_args(arg));
}

template<>
std::string string_utils::format<float, float>(std::string_view format_str, const float& arg1, const float& arg2) noexcept {

    return fmt::vformat(format_str, fmt::make_format_args(arg1, arg2));
}

template<>
std::string string_utils::format<std::string>(std::string_view format_str, const std::string& arg) noexcept {

    return fmt::vformat(format_str, fmt::make_format_args(arg));
}

template<>
std::string string_utils::format<std::string_view>(std::string_view format_str, const std::string_view& arg) noexcept {

    return fmt::vformat(format_str, fmt::make_format_args(arg));
}
