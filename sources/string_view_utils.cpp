#include <MazeBuilder/string_view_utils.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid_interface.h>

#if defined(MAZE_DEBUG)

#include <iostream>
#endif

#include <algorithm>
#include <cctype>
#include <random>
#include <sstream>
#include <tuple>
#include <unordered_set>
#include <list>

using namespace mazes;

/// @brief Strip specific characters from the beginning and end of a string view
/// @param s The string view to strip characters from
/// @param to_strip_from_s The character to strip " "
/// @return A new string view with the specified characters removed from both ends
std::string_view string_view_utils::strip(const std::string_view& s, const std::string_view& to_strip_from_s) noexcept {
    if (s.empty()) {
        return s;
    }
    
    auto start = s.find_first_not_of(to_strip_from_s);
    auto end = s.find_last_not_of(to_strip_from_s);
    
    if (start == std::string_view::npos || end == std::string_view::npos) {
        return s;
    }
    
    return s.substr(start, end - start + 1);
}

bool string_view_utils::contains(const std::string& str, const std::string& substr) noexcept {
    return str.find(substr) != std::string::npos;
}

std::string string_view_utils::get_file_extension(const std::string& filename) noexcept {
    auto pos = filename.find_last_of(".");
    if (pos == std::string::npos) {
        return "";
    }
    return filename.substr(pos);
}

bool string_view_utils::ends_with(const std::string& str, const std::string& suffix) noexcept {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

std::string_view string_view_utils::find_first_of(const std::string_view& s, const std::string_view& chars) noexcept {

    if (s.empty() || chars.empty()) {

        return {};
    }
    
    auto pos = s.find_first_of(chars);
    if (pos == std::string_view::npos) {

        return {};
    }
    
    return s.substr(pos, 1);
}

/// @brief Split a string by a delimiter
/// @param str 
/// @param delimiter ' '
/// @return 
std::list<std::string> string_view_utils::split(const std::string& str, char delimiter) noexcept {
    std::list<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

/// @brief Split a string_view by a delimiter
/// @param sv 
/// @param delim " "
/// @return 
std::list<std::string_view> string_view_utils::split(const std::string_view& sv, const std::string_view& delim) noexcept {
    std::list<std::string_view> result;
    
    if (sv.empty()) {
        return result;
    }
    
    if (delim.empty()) {
        result.push_back(sv);
        return result;
    }
    
    size_t start = 0;
    size_t found = sv.find(delim, start);
    
    while (found != std::string_view::npos) {
        // Add the part before the delimiter
        if (found > start) {
            result.push_back(sv.substr(start, found - start));
        }
        
        // Move past the delimiter
        start = found + delim.length();
        found = sv.find(delim, start);
    }
    
    // Add the remaining part after the last delimiter
    if (start < sv.length()) {
        result.push_back(sv.substr(start));
    }
    
    return result;
}

// Fix the implementation for the map version to be compatible with the args version
std::string string_view_utils::to_string(std::unordered_map<std::string, std::string> const& m) noexcept {
    // Check if the map is empty
    if (m.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    
    // Build a filtered map to avoid duplicate entries with different forms of the same key
    std::unordered_map<std::string, std::string> filtered_map;
    for (const auto& [key, value] : m) {
        // Only include keys without dashes to avoid duplicates
        if (!key.empty() && key[0] != '-') {
            filtered_map[key] = value;
        }
    }
    
    // If there are no non-dashed keys, return empty string
    if (filtered_map.empty()) {
        return "";
    }
    
    // Serialize the filtered map
    for (const auto& [key, value] : filtered_map) {
        oss << key << ": " << value << "\n";
    }
    
    return oss.str();
}
