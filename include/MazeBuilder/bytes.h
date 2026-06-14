#ifndef BYTES_H
#define BYTES_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

/// @file bytes.h
/// @namespace mazes
namespace mazes
{
    /// @class bytes
    /// @brief Base64 encoding and decoding helper class
    /// @details This class provides methods to encode and decode strings using the Base64 encoding scheme
    class bytes
    {
    public:
        /// @brief Transform an input string into base64 characters
        /// @param sv The input string to encode
        /// @return A string representing the base64 encoded input
        static std::string encode(std::string_view sv) noexcept;

        /// @brief Transform an input string from from base64 characters
        /// @param sv The base64 encoded string to decode
        /// @return A string representing the decoded input
        static std::string decode(std::string_view sv) noexcept;

        /// @brief Transform a vector of bytes into a string_view
        /// @param bytes The vector of bytes to transform
        /// @return A string_view representing the bytes
        static std::string_view bytes_to_string(const std::vector<std::uint8_t>& bytes) noexcept;

        /// @brief Transform a string_view into a vector of bytes
        /// @param sv The string_view to transform
        /// @return A vector of bytes representing the string_view
        static std::vector<std::uint8_t> string_to_bytes(std::string_view sv) noexcept;

        /// @brief Convert an 32-bit integer value to a base36 string
        /// @param value The 32-bit integer value to convert
        /// @return A string representing the value in base36
        static std::string to_base36(std::uint32_t value) noexcept;
    }; // class

} // namespace

#endif // BYTES_H
