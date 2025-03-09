#ifndef ARGS_H
#define ARGS_H

#include <string>
#include <memory>
#include <vector>
#include <ostream>

namespace mazes {

/// @file args.h
/// @class args
/// @brief Simple argument handler
struct args {
public:

    // Default constructor
    explicit args();

    // Destructor
    ~args();

    // Copy constructor
    args(const args& other);

    // Copy assignment operator
    args& operator=(const args& other);

    // Move constructor
    args(args&& other) noexcept = default;

    // Move assignment operator
    args& operator=(args&& other) noexcept = default;

    /// @brief Add a flag to the args map
    /// @param key 
    /// @param desc 
    void add_flag(const std::string& key, bool& r, const std::string& desc) noexcept;

    /// @brief Add an option to the args map
    /// @param key 
    /// @param desc 
    void add_option(const std::string& key, const std::string& desc) noexcept;

    /// @brief Parse program arguments from original args
    /// @param argc 
    /// @param argv 
    /// @return 
    bool parse(int argc, char* argv[]) noexcept;

    /// @brief Parse program arguments from a vector of strings
    /// @param arguments
    /// @return 
    bool parse(const std::vector<std::string>& arguments) noexcept;

    /// @brief Parse program arguments from a string
    /// @param arguments 
    /// @return 
    bool parse(const std::string& arguments) noexcept;

    /// @brief Clear the internal CLI program
    void clear() noexcept;

    /// @brief Display the arguments to a string output
    /// @return 
    friend std::ostream& operator<<(std::ostream& os, const args& a) noexcept;
private:
    std::string get(const std::string& key) const noexcept;

    struct args_impl;
    std::unique_ptr<args_impl> pimpl;
};

}
#endif // ARGS_H
