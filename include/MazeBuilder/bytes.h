#ifndef BYTES_H
#define BYTES_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mazes
{
    /// @file bytes.h
    /// @class bytes
    /// @brief Base64 encoding and decoding helper class
    /// @details This class provides methods to encode and decode strings using the Base64 encoding scheme
    class bytes
    {
    public:
        /// @brief Transform an input string into base64 characters
        /// @param sv
        /// @return
        static std::string encode(std::string_view sv) noexcept;

        /// @brief Transform an input string from from base64 characters
        static std::string decode(std::string_view sv) noexcept;

        static std::string_view bytes_to_string(const std::vector<std::uint8_t>& bytes) noexcept;

        static std::vector<std::uint8_t> string_to_bytes(std::string_view sv) noexcept;
    }; // class

} // namespace

#endif // BYTES_H
