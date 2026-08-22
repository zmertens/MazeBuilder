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
        static bool write(std::ostream& oss, std::string_view data, std::string_view delimiter = "\n") noexcept;

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
        static std::string parent_path(std::string_view filepath) noexcept;

        /// @brief Read an entire file into memory
        /// @param file_path Full file path
        /// @return The file contents as bytes, or an empty vector on failure
        static std::vector<std::uint8_t> read_file_to_bytes(const std::filesystem::path& file_path) noexcept;

        /// @brief Writes pixel data to a PNG image file.
        /// @param file_path The filesystem path where the PNG file will be written.
        /// @param pixels A vector containing the pixel data as 8-bit unsigned integers.
        /// @param width The width of the image in pixels.
        /// @param height The height of the image in pixels.
        /// @return True if the PNG file was successfully written, false otherwise.
        static bool write_png(const std::filesystem::path& file_path, const std::vector<std::uint8_t>& pixels, int width, int height) noexcept;

        /// @brief Writes image data to a JPEG file.
        /// @param file_path The file system path where the JPEG file will be written.
        /// @param pixels A vector containing the pixel data in RGB or RGBA format.
        /// @param width The width of the image in pixels.
        /// @param height The height of the image in pixels.
        /// @param quality The JPEG compression quality (0-100, default is 95). Higher values produce better quality but larger files.
        /// @return True if the JPEG file was successfully written, false otherwise.
        static bool write_jpg(const std::filesystem::path& file_path, const std::vector<std::uint8_t>& pixels, int width, int height, int quality = 95) noexcept;

        /// @brief Writes a BMP image file from pixel data.
        /// @param file_path The path where the BMP file will be written.
        /// @param pixels A vector containing the pixel data to write.
        /// @param width The width of the image in pixels.
        /// @param height The height of the image in pixels.
        /// @return Returns true if the BMP file was successfully written, false otherwise.
        static bool write_bmp(const std::filesystem::path& file_path, const std::vector<std::uint8_t>& pixels, int width, int height) noexcept;
    }; // io_utils
}

#endif // IO_UTILS_H
