#ifndef BASE64_HELPER_H
#define BASE64_HELPER_H

#include <memory>
#include <string>

namespace mazes {

class base_64_helper {
public:
    explicit base_64_helper();

    ~base_64_helper();

    // Copy constructor
    base_64_helper(const base_64_helper& other);

    // Copy assignment operator
    base_64_helper& operator=(const base_64_helper& other);

    // Move constructor
    base_64_helper(base_64_helper&& other) noexcept = default;

    // Move assignment operator
    base_64_helper& operator=(base_64_helper&& other) noexcept = default;

    /// @brief 
    /// @param encode 
    /// @param result 
    void encode(const std::string& encode, std::string& result) const noexcept;

    /// @brief 
    /// @param decode 
    /// @param result 
    void decode(const std::string& decode, std::string& result) const noexcept;
	
private:
	class base_64_helper_impl;
	std::unique_ptr<base_64_helper_impl> impl;
}; // class

} // namespace

#endif // BASE64_HELPER_H
