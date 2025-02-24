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
    static constexpr auto ArgsPattern = R"pattern(\.*)pattern";

    /// @brief Parse program arguments
    /// @example
    ///     auto args = mazes::args{};
    ///     auto success = args.parse({"-r", "10", "-c", "10", "-s", "2"});
    /// @param arguments
    /// @return 
    bool parse(const std::vector<std::string>& arguments) noexcept;

    /// @brief Parse program arguments
    /// @example
    ///     auto args = mazes::args{};
    ///     auto success = args.parse("-r 10 -c 10 -s 2");
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

    /// @brief Dump the hash table to a string output
    /// @return 
    friend std::ostream& operator<<(std::ostream& os, const args& a) noexcept;
public:
    std::unordered_map<std::string, std::string> args_map;
};

}
#endif // ARGS_H
