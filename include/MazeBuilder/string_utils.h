#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <algorithm>
#include <string>
#include <string_view>
#include <type_traits>

/// @file string_utils.h
/// @namespace mazes
namespace mazes
{
    /// @class string_utils
    /// @brief String helper class
    /// @details This class provides common string manipulation utilities
    /// @related https://github.com/PacktPublishing/CPP-20-STL-Cookbook/blob/main/chap11/split.cpp
    class string_utils
    {
    private:
        // Helper trait to detect if a type has push_back method - local to this function
        template <typename T, typename = void>
        struct has_push_back : std::false_type
        {
        };

        template <typename T>
        struct has_push_back<T, std::void_t<decltype(std::declval<T>().push_back(std::declval<typename T::value_type>()))>> : std::true_type
        {
        };

        /// @brief Default equality predicate for split functions
        static constexpr auto eq = [](const auto& el, const auto& sep) -> bool
            {
                using std::is_convertible_v;
                using std::is_same_v;

                using ElType = decltype(el);
                using SepType = decltype(sep);

                if constexpr (is_same_v<ElType, SepType>)
                {
                    return el == sep;
                } else if constexpr (is_convertible_v<ElType, SepType>)
                {
                    return static_cast<SepType>(el) == sep;
                } else if constexpr (is_convertible_v<SepType, ElType>)
                {
                    return el == static_cast<ElType>(sep);
                } else
                {
                    return false;
                }
            };

    public:
        /// @brief Combine and return two strings
        /// @param a The first string
        /// @param b The second string
        /// @return A representation of the concatenated string
        static std::string concat(const std::string& a, const std::string& b) noexcept;

        /// @brief Check if a string contains a substring
        /// @param str The string to search in
        /// @param substr The substring to search for
        /// @return True if substr is found in str, false otherwise
        static bool contains(const std::string& str, const std::string& substr) noexcept;

        /// @brief Check if a string ends with a specific suffix
        /// @param str The string to check
        /// @param suffix The suffix to check for
        /// @return True if str ends with suffix, false otherwise
        static bool ends_with(const std::string& str, const std::string& suffix) noexcept;

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
        template <typename It, typename Oc, typename V, typename Pred>
        static It split(It it, const It end_it, Oc& dest, const V& sep, Pred f)
        {
            using std::is_same_v;
            using std::string;

            using SliceContainer = typename Oc::value_type;

            while (it != end_it)
            {
                SliceContainer dest_elm{};

                auto slice{ it };

                while (slice != end_it)
                {
                    if (f(*slice, sep))
                        break;

                    // Handle string vs other containers
                    if constexpr (is_same_v<SliceContainer, string>)
                    {
                        dest_elm += *slice;
                    } else if constexpr (has_push_back<SliceContainer>::value)
                    {
                        dest_elm.push_back(*slice);
                    } else
                    {
                        // For types without push_back, provide a helpful error
                        static_assert(is_same_v<SliceContainer, string> || has_push_back<SliceContainer>::value,
                            "SliceContainer must be std::string or have a push_back method");
                    }
                    ++slice;
                }

                dest.push_back(dest_elm);

                if (slice == end_it)
                {
                    return end_it;
                }

                it = ++slice;
            }
            return it;
        }

        /// @brief Generic split function with default equality predicate
        /// @tparam It Iterator type
        /// @tparam Oc Output container type
        /// @tparam V Value type
        /// @param it Start iterator
        /// @param end_it End iterator
        /// @param dest Output container
        /// @param sep Separator value
        /// @return Iterator to end position
        template <typename It, typename Oc, typename V>
        static It split(It it, const It end_it, Oc& dest, const V& sep)
        {
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
        template <typename Cin, typename Cout, typename V>
        static Cout& strsplit(const Cin& str, Cout& dest, const V& sep)
        {
            split(str.begin(), str.end(), dest, sep, eq);
            return dest;
        }

        /// @brief Checks if a character is a whitespace character.
        /// @tparam T The type of the character to check.
        /// @param c The character to check for whitespace.
        /// @return True if the character is a whitespace character; otherwise, false.
        template <typename T>
        static bool is_whitespace(const T& c)
        {
            using std::is_same_v;
            using std::string_view;

            // Use std::string_view for safer character comparison
            constexpr string_view whitespace_chars = " \t\r\n\v\f";

            // For character types, convert to char for comparison
            if constexpr (is_same_v<T, char>)
            {
                return whitespace_chars.find(c) != string_view::npos;
            } else
            {
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
        static std::string_view strip_whitespace(const std::string& s)
        {
            std::string_view output_str{ s };
            output_str.remove_prefix(std::min(output_str.find_first_not_of(" \t\r\n\v\f"), output_str.size()));
            return output_str;
        }

        static std::string_view strip_backticks(const std::string& s)
        {
            std::string_view output_str{ s };
            output_str.remove_prefix(std::min(output_str.find_first_not_of("\\`"), output_str.size()));
            output_str.remove_suffix(std::min(output_str.size() - output_str.find_last_not_of("\\`") - 1, output_str.size()));
            return output_str;
        }

        static std::string replace_all(const std::string& s, const std::string& from, const std::string& to)
        {
            if (from.empty())
            {
                return s;
            }

            std::string result{ s };
            size_t start_pos = 0;
            while ((start_pos = result.find(from, start_pos)) != std::string::npos)
            {
                result.replace(start_pos, from.length(), to);
                start_pos += to.length(); // Move past the replaced part
            }
            return result;
        }

        static std::string file_extension(std::string_view filename) noexcept;
    }; // class
} // namespace

#endif // STRING_UTILS_H
