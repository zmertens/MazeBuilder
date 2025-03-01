#ifndef ARGS_H
#define ARGS_H

#include <string>
#include <unordered_map>
#include <vector>
#include <ostream>

namespace mazes {

/// @brief Simple argument handler
struct args {
public:
    /// @brief Regular expression pattern for checking arguments
    static constexpr auto ArgsPattern = R"pattern([\-\.\=\w\s\d]+)pattern";

    /// @brief Parse program arguments from a vector of strings
    /// @param arguments
    /// @return 
    bool parse(const std::vector<std::string>& arguments) noexcept;

    /// @brief Parse program arguments from a string
    /// @param arguments 
    /// @return 
    bool parse(const std::string& arguments) noexcept;

    /// @brief Get a value from the args map
    /// @param key 
    /// @return 
    std::string get(const std::string& key) const noexcept;

    /// @brief Get entire the args map
    /// @return 
    const std::unordered_map<std::string, std::string>& get() const noexcept;

    /// @brief Display the arguments to a string output
    /// @return 
    friend std::ostream& operator<<(std::ostream& os, const args& a) noexcept;
public:
    std::unordered_map<std::string, std::string> args_map;
};

}
#endif // ARGS_H
