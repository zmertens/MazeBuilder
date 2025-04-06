#ifndef JSON_HELPER_H
#define JSON_HELPER_H

#include <string>
#include <memory>
#include <unordered_map>

namespace mazes {

/// @file json_helper.h
/// @class json_helper
/// @brief JSON helper class
/// @details This class provides methods to convert a map of strings into a JSON string
class json_helper {
public:
    /// @brief Default constructor
    explicit json_helper();

    /// @brief Destructor
    ~json_helper();

    // Copy constructor
    json_helper(const json_helper& other);

    // Copy assignment operator
    json_helper& operator=(const json_helper& other);

    // Move constructor
    json_helper(json_helper&& other) noexcept = default;

    // Move assignment operator
    json_helper& operator=(json_helper&& other) noexcept = default;

    /// @brief Get the contents of a map as a string in JSON format
    /// @param map 
    /// @param pretty_print Number of spaces to use for indenting the JSON string
    /// @return 
    std::string from(const std::unordered_map<std::string, std::string>& map, int pretty_print = 4) const noexcept;

    /// @brief Parse and set a JSON string into a C++ map
    /// @param s 
    /// @param m 
    /// @return success or failure on parse
    bool from(const std::string& s, std::unordered_map<std::string, std::string>& m) const noexcept;

    /// @brief Parse a JSON file into a C++ map from a file on disk
    /// @param filename 
    /// @param m 
    /// @return 
    bool load(const std::string& filename, std::unordered_map<std::string, std::string>& m) const noexcept;

private:
    /// @brief Forward declaration of the implementation class
    class json_helper_impl;
    std::unique_ptr<json_helper_impl> impl;
};

} // namespace mazes

#endif // JSON_HELPER_H
