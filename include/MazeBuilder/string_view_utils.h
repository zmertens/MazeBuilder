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

namespace mazes {

class args;
class maze;
class lab;

/// @file string_view_utils.h

/// @class string_view_utils
/// @brief String helper class
/// @details This class provides methods to convert mazes into string representations
class string_view_utils {
public:

    /// @brief Compute the 3D geometries of a maze
    /// @param m the maze to convert
    /// @param vertices the vertices of the maze
    /// @param faces the faces of the maze
    /// @param s the string representation of the maze
    static void objectify(const std::unique_ptr<maze>& m,
        std::vector<std::tuple<int, int, int, int>>& vertices,
        std::vector<std::vector<std::uint32_t>>& faces,
        std::string_view sv = std::string_view{}) noexcept;

    /// @brief Compute the 3D geometries of a maze
    /// @param labyrinth the collection of mazes
    /// @param sv 
    static void objectify(lab& labyrinth,
        std::string_view sv = std::string_view{}) noexcept;

    /// @brief Converts a string representation of an image to a pixel array.
    /// @param s 
    /// @param pixels vector to store the resulting pixel data
    /// @param width integer reference to store the width of the image
    /// @param height integer reference to store the height of the image
    static void to_pixels(const std::string& s, std::vector<std::uint8_t>& pixels, int& width, int& height,
        int stride = 4) noexcept;

    /// @brief Converts a maze to a pixel representation.
    /// @param m A unique pointer to the maze object to be converted.
    /// @param pixels A vector to store the resulting pixel data.
    /// @param width An integer reference to store the width of the pixel data.
    /// @param height An integer reference to store the height of the pixel data.
    /// @param stride The number of bytes per row in the pixel data.
    static void to_pixels_colored(const std::string& s, std::vector<std::uint8_t>& pixels, int& width, int& height, int stride = 4) noexcept;

    /// @brief Convert a maze into a string representation
    /// @param m the maze to convert
    /// @return the string representation
    static std::string stringify(const std::unique_ptr<maze>& m) noexcept;

    /// @brief Trim whitespace from both ends of a string
    /// @param str The string to trim
    /// @return A trimmed copy of the string
    static std::string trim(const std::string& str) noexcept;
    
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
    /// @return Vector of split substrings
    static std::vector<std::string> split(const std::string& str, char delimiter) noexcept;

    /// @brief Strip specific characters from the beginning and end of a string view
    /// @param s The string view to strip characters from
    /// @param to_strip_from_s The character to strip
    /// @return A new string view with the specified characters removed from both ends
    static std::string_view strip(const std::string_view& s, char to_strip_from_s) noexcept;

    /// @brief Strip quotes from a JSON string value
    /// @param s The string view potentially containing JSON quotes
    /// @return A string view with JSON quotes removed from both ends
    static std::string_view strip_json_quotes(const std::string_view& s) noexcept;


    /// @brief Convert a map to a formatted string with each key-value pair on a line
    /// @param m The map to convert
    /// @return A formatted string representation of the map
    static std::string to_string(std::unordered_map<std::string, std::string> const& m) noexcept;

    // Add these prototypes before the template method
    /// @brief Convert an args object to a string
    /// @param a The args object to convert
    /// @return A formatted string representation of the args
    static std::string to_string(const args& a) noexcept;

    /// @brief Convert a reference_wrapper of args to a string
    /// @param a The reference_wrapper of args to convert
    /// @return A formatted string representation of the args
    static std::string to_string(const std::reference_wrapper<const args>& a) noexcept;

}; // class
} // namespace

#endif // STRINGZ_H
