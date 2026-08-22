#include <MazeBuilder/io_utils.h>

#include <filesystem>
#include <fstream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include <stb/stb_image_write.h>

using namespace mazes;

// delimiter = "\n"
bool io_utils::write(std::ostream& oss, std::string_view data, std::string_view delimiter) noexcept
{
    oss << data << delimiter;

    return oss.good();
}

bool io_utils::write_file(std::string_view filename, std::string_view data) noexcept
{
    if (filename.empty())
    {
        return false;
    }

    std::filesystem::path data_path{ filename };

    // Resolve relative paths against the current working directory instead of rejecting them.
    if (!data_path.is_absolute())
    {
        try
        {
            data_path = std::filesystem::absolute(data_path);
        } catch (const std::exception&)
        {
            return false;
        }
    }

    std::ofstream out_writer{ data_path };

    if (!out_writer.is_open())
    {
        return false;
    }

    out_writer << data;

    out_writer.close();

    return out_writer.good();
}

bool io_utils::is_valid_path(std::string_view path) noexcept
{
    try
    {
        std::filesystem::path p{ path };
        return !p.empty();
    } catch (const std::exception&)
    {
        return false;
    }
}

bool io_utils::is_an_absolute_path(std::string_view path) noexcept
{
    std::filesystem::path p(path);

    return p.is_absolute();
}

std::string io_utils::parent_path(std::string_view filepath) noexcept
{
    const std::filesystem::path p(filepath);

    return p.parent_path().string();
}

std::vector<std::uint8_t> io_utils::read_file_to_bytes(const std::filesystem::path& file_path) noexcept
{
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return {};
    }

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(size));
    if (size > 0 && !file.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        return {};
    }

    return buffer;
}

bool io_utils::write_png(const std::filesystem::path& file_path, const std::vector<std::uint8_t>& pixels, int width, int height) noexcept
{
    return stbi_write_png(file_path.string().c_str(), width, height, 4, pixels.data(), width * 4) != 0;
}

bool io_utils::write_jpg(const std::filesystem::path& file_path, const std::vector<std::uint8_t>& pixels, int width, int height, int quality) noexcept
{
    return stbi_write_jpg(file_path.string().c_str(), width, height, 4, pixels.data(), quality) != 0;
}

bool io_utils::write_bmp(const std::filesystem::path & file_path, const std::vector<std::uint8_t>&pixels, int width, int height) noexcept
{
    return stbi_write_bmp(file_path.string().c_str(), width, height, 4, pixels.data()) != 0;
}
