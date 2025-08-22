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

// Fix the implementation for the map version to be compatible with the args version
std::string string_utils::to_string(std::unordered_map<std::string, std::string> const& m) noexcept {
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
