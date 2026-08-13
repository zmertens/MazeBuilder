#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <cstdint>
#include <filesystem>
#include <ostream>
#include <string_view>
#include <vector>

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
        static bool write(std::ostream &oss, std::string_view data, std::string_view delimiter = "\n") noexcept;

        /// @brief Write to a file
        /// @param filename Full file path
        /// @param data Data to write
        /// @return True if the write was successful, false otherwise
        static bool write_file(std::string_view filename, std::string_view data) noexcept;

        /// @brief Check if a path is an absolute path
        /// @param path Path to check
        static bool is_an_absolute_path(std::string_view path) noexcept;

        /// @brief Check if a path is valid
        /// @param path
        /// @return
        static bool is_valid_path(std::string_view path) noexcept;

        /// @brief Get the directory path from a full file path
        /// @param filepath Full file path
        /// @return Directory path
        static std::string_view get_full_directory_path(std::string_view filepath) noexcept;

        /// @brief Read an entire file into memory
        /// @param file_path Full file path
        /// @return The file contents as bytes, or an empty vector on failure
        static std::vector<std::uint8_t> read_file_to_bytes(const std::filesystem::path &file_path) noexcept;
    }; // io_utils
}

#endif // IO_UTILS_H
