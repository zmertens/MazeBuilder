#ifndef BASE64_HELPER_H
#define BASE64_HELPER_H

#include <memory>
#include <string>
#include <string_view>

namespace mazes {

/// @file base64_helper.h
/// @class base64_helper
/// @brief Base64 encoding and decoding helper class
/// @details This class provides methods to encode and decode strings using the Base64 encoding scheme
class base64_helper {
public:

    /// @brief Transform an input string into base64 characters
    /// @param sv
    /// @return 
    static std::string encode(std::string_view sv) noexcept;

    /// @brief Transform an input string from from base64 characters
    static std::string decode(std::string_view sv) noexcept;

private:

}; // class

} // namespace

#endif // BASE64_HELPER_H
