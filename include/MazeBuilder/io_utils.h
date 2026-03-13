#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>

namespace mazes
{

    /// @file io_utils.h
    /// @class io_utils
    /// @brief Handles file writing for text, stdout, and object files
    class io_utils
    {
    public:
        /// @brief Handles writing to an output stream
        /// @param oss
        /// @param data
        /// @return
        bool write(std::ostream &oss, const std::string &data, std::string_view delimiter = "\n") const noexcept;

        /// @brief Write to a file
        /// @param filename
        /// @param data
        /// @return
        bool write_file(const std::string &filename, const std::string &data) const noexcept;

        /// @brief Get the directory path from a full file path
        /// @param filepath Full file path
        /// @return Directory path
        static std::string getDirectoryPath(const std::string &filepath) noexcept
        {
            std::filesystem::path p(filepath);
            return p.parent_path().string();
        }
    }; // io_utils

}

#endif // IO_UTILS_H
