#ifndef STRINGZ_H
#define STRINGZ_H

#include <cstdint>
#include <memory>
#include <string_view>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <list>

namespace mazes {

/// @file string_view_utils.h

/// @class string_view_utils
/// @brief String helper class
/// @details This class provides common string manipulation utilities
class string_view_utils {
public:
    
    /// @brief Check if a string contains a substring
    /// @param str The string to search in
    /// @param substr The substring to search for
    /// @return True if substr is found in str, false otherwise
    static bool contains(const std::string& str, const std::string& substr) noexcept;
    
    /// @brief Extract file extension from a filename
    /// @param filename The filename to process
    /// @return The file extension including the dot, or empty string if no extension
    static std::string get_file_extension(const std::string& filename) noexcept;
    
    /// @brief Check if a string ends with a specific suffix
    /// @param str The string to check
    /// @param suffix The suffix to check for
    /// @return True if str ends with suffix, false otherwise
    static bool ends_with(const std::string& str, const std::string& suffix) noexcept;

    /// @brief Find the first occurrence of any character from a set in a string view
    /// @param s The string view to search in
    /// @param chars The set of characters to search for
    /// @return A string view starting from the first occurrence of any character in chars, or the end of s if none found
    static std::string_view find_first_of(const std::string_view& s, const std::string_view& chars) noexcept;
    
    /// @brief Split a string by delimiter
    /// @param str The string to split
    /// @param delimiter The delimiter to split by
    /// @return List of split substrings
    static std::list<std::string> split(const std::string& str, char delimiter = ' ') noexcept;

    /// @brief Split a string_view by delimiter
    /// @param sv The string_view to split
    /// @param delim The delimiter to split by (defaults to space)
    /// @return List of string_view parts
    static std::list<std::string_view> split(const std::string_view& sv, const std::string_view& delim = " ") noexcept;

    /// @brief Strip specific characters from the beginning and end of a string view
    /// @param s The string view to strip characters from
    /// @param to_strip_from_s The character to strip
    /// @return A new string view with the specified characters removed from both ends
    static std::string_view strip(const std::string_view& s, const std::string_view& to_strip_from_s = " ") noexcept;

    /// @brief Convert a map to a formatted string with each key-value pair on a line
    /// @param m The map to convert
    /// @return A formatted string representation of the map
    static std::string to_string(std::unordered_map<std::string, std::string> const& m) noexcept;

}; // class
} // namespace

#endif // STRINGZ_H
