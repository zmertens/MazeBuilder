#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <list>
#include <memory>
#include <sstream>
#include <string_view>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace mazes {

/// @file string_utils.h

/// @class string_utils
/// @brief String helper class
/// @details This class provides common string manipulation utilities
/// @related https://github.com/PacktPublishing/CPP-20-STL-Cookbook/blob/main/chap11/split.cpp
class string_utils {
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

    // Template-based split functions

    // Helper trait to detect if a type has push_back method - local to this function
    template<typename T, typename = void>
    struct has_push_back : std::false_type {};

    template<typename T>
    struct has_push_back<T, std::void_t<decltype(std::declval<T>().push_back(std::declval<typename T::value_type>()))>> : std::true_type {};


    /// @brief Splits a range into slices based on a separator and stores the results in a destination container.
    /// @tparam It Type of the iterator for the input range.
    /// @tparam Oc Type of the output container that will store the slices.
    /// @tparam V Type of the separator value.
    /// @tparam Pred Type of the predicate function used to compare elements to the separator.
    /// @param it Iterator pointing to the beginning of the range to split.
    /// @param end_it Iterator pointing to the end of the range to split.
    /// @param dest Destination container where the resulting slices will be stored.
    /// @param sep Separator value used to determine where to split the range.
    /// @param f Predicate function that determines if an element matches the separator.
    /// @return Iterator pointing to the position after the last processed element, or end_it if the entire range was processed.
    template<typename It, typename Oc, typename V, typename Pred>
    static It split(It it, const It end_it, Oc& dest, const V& sep, Pred f) {

        using std::is_same_v;
        using std::string;

        using SliceContainer = typename Oc::value_type;

        while (it != end_it) {

            SliceContainer dest_elm{};

            auto slice{ it };

            while (slice != end_it) {

                if (f(*slice, sep)) break;

                // Handle string vs other containers
                if constexpr (is_same_v<SliceContainer, string>) {

                    dest_elm += *slice;
                } else if constexpr (has_push_back<SliceContainer>::value) {

                    dest_elm.push_back(*slice);
                } else {

                    // For types without push_back, provide a helpful error
                    static_assert(is_same_v<SliceContainer, string> || has_push_back<SliceContainer>::value,
                        "SliceContainer must be std::string or have a push_back method");
                }
                ++slice;
            }

            dest.push_back(dest_elm);

            if (slice == end_it) {

                return end_it;
            }

            it = ++slice;
        }
        return it;
    }

    /// @brief Default equality predicate for split functions
    static constexpr auto eq = [](const auto& el, const auto& sep) -> bool {

        using std::is_convertible_v;
        using std::is_same_v;

        using ElType = std::decay_t<decltype(el)>;
        using SepType = std::decay_t<decltype(sep)>;

        if constexpr (is_same_v<ElType, SepType>) {

            return el == sep;
        } else if constexpr (is_convertible_v<ElType, SepType>) {

            return static_cast<SepType>(el) == sep;
        } else if constexpr (is_convertible_v<SepType, ElType>) {

            return el == static_cast<ElType>(sep);
        } else {

            return false;
        }
        };

    /// @brief Generic split function with default equality predicate
    /// @tparam It Iterator type
    /// @tparam Oc Output container type
    /// @tparam V Value type
    /// @param it Start iterator
    /// @param end_it End iterator
    /// @param dest Output container
    /// @param sep Separator value
    /// @return Iterator to end position
    template<typename It, typename Oc, typename V>
    static It split(It it, const It end_it, Oc& dest, const V& sep) {
        return split(it, end_it, dest, sep, eq);
    }

    /// @brief High-level string split function using containers
    /// @tparam Cin Input container type
    /// @tparam Cout Output container type
    /// @tparam V Value type
    /// @param str Input container/string
    /// @param dest Output container
    /// @param sep Separator value
    /// @return Reference to output container
    template<typename Cin, typename Cout, typename V>
    static Cout& strsplit(const Cin& str, Cout& dest, const V& sep) {
        split(str.begin(), str.end(), dest, sep, eq);
        return dest;
    }


    /// @brief Checks if a character is a whitespace character.
    /// @tparam T The type of the character to check.
    /// @param c The character to check for whitespace.
    /// @return True if the character is a whitespace character; otherwise, false.
    template<typename T>
    static bool isWhitespace(const T& c) {

        using std::is_same_v;
        using std::string_view;

        // Use std::string_view for safer character comparison
        constexpr string_view whitespace_chars = " \t\r\n\v\f";

        // For character types, convert to char for comparison
        if constexpr (is_same_v<T, char>) {
            return whitespace_chars.find(c) != string_view::npos;
        } else {
            // For other types, do individual comparisons
            return c == static_cast<T>(' ') || 
                    c == static_cast<T>('\t') || 
                    c == static_cast<T>('\r') || 
                    c == static_cast<T>('\n') || 
                    c == static_cast<T>('\v') || 
                    c == static_cast<T>('\f');
        }
    }

    /// @brief Removes consecutive whitespace characters from a string, leaving only single whitespace between non-whitespace characters.
    /// @param s The input string from which to strip consecutive whitespace.
    /// @return A new string with consecutive whitespace characters replaced by a single whitespace.
    static std::string stripWhitespace(const std::string& s) {

        using std::string;
        using std::unique;

        string outputString{ s };

        auto its = unique(outputString.begin(), outputString.end(),
            [](const auto& a, const auto& b) {

                return isWhitespace(a) && isWhitespace(b);
            });

        outputString.erase(its, outputString.end());

        outputString.shrink_to_fit();

        return outputString;
    }

}; // class

} // namespace

#endif // STRING_UTILS_H
