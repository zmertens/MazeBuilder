#ifndef WRITER_H
#define WRITER_H

#include <string>

namespace mazes {

/// @brief Handles file writing for text, stdout, images, and object files
class writer {
public:

	/// @brief Handles writing to an image file
	/// @param filename 
	/// @param data the string to write to a file
    /// @param w 1 width in pixels
    /// @param h 1 height in pixels
	/// @return 
	bool write(const std::string& filename, const std::string& data, unsigned int w = 1, unsigned int h = 1) const noexcept;

    /// @brief Handles writing to an output stream
    /// @param oss 
    /// @param data 
    /// @return 
    bool write(std::ostream& oss, const std::string& data) const noexcept;

private:
    bool write_jpeg(const std::string& filename, const std::string& data, unsigned int w = 1, unsigned int h = 1) const noexcept;
    bool write_png(const std::string& filename, const std::string& data, unsigned int w = 1, unsigned int h = 1) const noexcept;
    bool write_file(const std::string& filename, const std::string& data) const noexcept;
}; // writer

}

#endif // WRITER_H
