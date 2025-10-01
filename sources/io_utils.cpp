#include <MazeBuilder/io_utils.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <MazeBuilder/enums.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

using namespace mazes;

/// @brief Write pixels to a PNG file
/// @param filename
/// @param data
/// @param w 100
/// @param h 100
/// @param stride 4
/// @return
bool io_utils::write_png(const std::string &filename, const std::vector<std::uint8_t> &data, unsigned int w, unsigned int h, unsigned int stride) const noexcept
{

    return (0 != stbi_write_png(filename.c_str(), w, h, stride, data.data(), w * stride));
}

/// @brief Write pixels to a JPEG file
/// @param filename
/// @param data
/// @param w 100
/// @param h 100
/// @param stride 4
/// @return
bool io_utils::write_jpeg(const std::string &filename, const std::vector<std::uint8_t> &data, unsigned int w, unsigned int h, unsigned int stride) const noexcept
{

    return (0 != stbi_write_jpg(filename.c_str(), w, h, stride, data.data(), w * stride));
}

/// @brief Write to a conventional file
/// @param filename
/// @param data
/// @return
bool io_utils::write_file(const std::string &filename, const std::string &data) const noexcept
{
    using namespace std;

    filesystem::path data_path{filename};

    ofstream out_writer{data_path};

    if (!out_writer.is_open())
    {
        return false;
    }

    out_writer << data;

    out_writer.close();

    return out_writer.good();
}

bool io_utils::write(std::ostream &oss, const std::string &data) const noexcept
{
    using namespace std;

    oss << data << "\n";

    return oss.good();
}
