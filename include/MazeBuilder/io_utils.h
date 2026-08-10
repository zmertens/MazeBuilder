#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>

/// @file io_utils.h
/// @namespace mazes
namespace mazes
{
    /// @class io_utils
    /// @brief Handles file writing for text, stdout, and object files
    class io_utils
    {
    public:
        /// @brief Handles writing to an output stream
        /// @param oss
        /// @param data
        /// @param delimiter Delimiter to use between data entries
        /// @return True if the write was successful, false otherwise
        static bool write(std::ostream& oss, const std::string& data, std::string_view delimiter = "\n") noexcept;

        /// @brief Write to a file
        /// @param filename Full file path
        /// @param data Data to write
        /// @return True if the write was successful, false otherwise
        static bool write_file(const std::string& filename, const std::string& data) noexcept;

        /// @brief Expand a leading '~' to the current user's home directory.
        /// @param path Path to normalize
        /// @return Path with '~' expanded when applicable
        static std::string normalize_path(const std::string& path) noexcept;

        /// @brief Get the directory path from a full file path
        /// @param filepath Full file path
        /// @return Directory path
        static std::string get_full_directory_path(const std::string& filepath) noexcept;
    }; // io_utils
}

#endif // IO_UTILS_H
