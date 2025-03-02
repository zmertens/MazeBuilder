#ifndef BASE64_HELPER_H
#define BASE64_HELPER_H

#include <memory>
#include <string>

namespace mazes {

/// @file base64_helper.h
/// @class base_64_helper
/// @brief Base64 encoding and decoding helper class
/// @details This class provides methods to encode and decode strings using the Base64 encoding scheme
class base_64_helper {
public:
    // Default constructor
    explicit base_64_helper();
    // Destructor
    ~base_64_helper();

    // Copy constructor
    base_64_helper(const base_64_helper& other);

    // Copy assignment operator
    base_64_helper& operator=(const base_64_helper& other);

    // Move constructor
    base_64_helper(base_64_helper&& other) noexcept = default;

    // Move assignment operator
    base_64_helper& operator=(base_64_helper&& other) noexcept = default;

    /// @brief Transform an input string into base64 characters
    /// @param s
    /// @return 
    std::string encode(const std::string& s) const noexcept;

    /// @brief Transform an input string from from base64 characters
    std::string decode(const std::string& s) const noexcept;
	
private:
	/// @brief Forward declaration of the implementation class
	class base_64_helper_impl;
	std::unique_ptr<base_64_helper_impl> impl;
}; // class

} // namespace

#endif // BASE64_HELPER_H
