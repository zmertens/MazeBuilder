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

using namespace mazes;

/// @brief Strip specific characters from the beginning and end of a string view
/// @param s The string view to strip characters from
/// @param to_strip_from_s The character to strip
/// @return A new string view with the specified characters removed from both ends
std::string_view string_view_utils::strip(const std::string_view& s, char to_strip_from_s) noexcept {
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

/// @brief Strip quotes from a JSON string value
/// @param s The string view potentially containing JSON quotes
/// @return A string view with JSON quotes removed from both ends
std::string_view string_view_utils::strip_json_quotes(const std::string_view& s) noexcept {
    if (s.empty()) {
        return s;
    }
    
    // Debug print to see the exact string content
#if defined(MAZE_DEBUG)

    std::cout << "Original string: [" << s << "]" << std::endl;
#endif
    
    std::string_view result = s;
    
    // First remove any leading/trailing whitespace
    auto start = result.find_first_not_of(" \t\n\r\"");
    auto end = result.find_last_not_of(" \t\n\r\"");
    
    if (start == std::string_view::npos || end == std::string_view::npos) {
        return result;
    }
    
    result = result.substr(start, end - start + 1);
    
    // Now check if the trimmed string starts and ends with quotes
    if (result.size() >= 2 && result.front() == '\"' && result.back() == '\"') {
        result = result.substr(1, result.size() - 2);
    }
    
    // Handle escaped quotes within the string if needed
    // (This would require more complex parsing if the JSON has escaped quotes)
    
    return strip(result, '\"');
}

std::string string_view_utils::trim(const std::string& str) noexcept {
    if (str.empty()) {
        return str;
    }
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
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

std::vector<std::string> string_view_utils::split(const std::string& str, char delimiter) noexcept {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
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
