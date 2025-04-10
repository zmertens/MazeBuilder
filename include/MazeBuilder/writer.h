#ifndef WRITER_H
#define WRITER_H

#include <string>
#include <vector>
#include <cstdint>

namespace mazes {

/// @file writer.h
/// @brief Handles writing to files, stdout, and images

/// @class writer
/// @brief Handles file writing for text, stdout, images, and object files
class writer {
public:

    /// @brief Write pixels to a PNG file
    /// @param filename 
    /// @param data 
    /// @param w 100
    /// @param h 100
    /// @param stride 4
    /// @return 
    bool write_png(const std::string& filename, const std::vector<std::uint8_t>& data, unsigned int w = 100, unsigned int h = 100, unsigned int stride = 4) const noexcept;

    /// @brief Write pixels to a JPEG file
    /// @param filename 
    /// @param data 
    /// @param w 100
    /// @param h 100
    /// @param stride 4
    /// @return 
    bool write_jpeg(const std::string& filename, const std::vector<std::uint8_t>& data, unsigned int w = 100, unsigned int h = 100, unsigned int stride = 4) const noexcept;

    /// @brief Handles writing to an output stream
    /// @param oss 
    /// @param data 
    /// @return 
    bool write(std::ostream& oss, const std::string& data) const noexcept;

    /// @brief Write to a file
    /// @param filename
    /// @param data 
    /// @return 
    bool write_file(const std::string& filename, const std::string& data) const noexcept;
}; // writer

}

#endif // WRITER_H
