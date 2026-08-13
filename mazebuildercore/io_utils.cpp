#include <MazeBuilder/io_utils.h>

#include <filesystem>
#include <fstream>

using namespace mazes;

// delimiter = "\n"
bool io_utils::write(std::ostream &oss, std::string_view data, std::string_view delimiter) noexcept
{
    oss << data << delimiter;

    return oss.good();
}

bool io_utils::write_file(std::string_view filename, std::string_view data) noexcept
{
    if (!is_an_absolute_path(filename))
    {
        return false;
    }

    std::filesystem::path data_path{filename};

    std::ofstream out_writer{data_path};

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
        std::filesystem::path p{path};
        return !p.empty();
    }
    catch (const std::exception &)
    {
        return false;
    }
}

bool io_utils::is_an_absolute_path(std::string_view path) noexcept
{
    std::filesystem::path p(path);

    return p.is_absolute();
}

std::string_view io_utils::get_full_directory_path(std::string_view filepath) noexcept
{
    const std::filesystem::path p(filepath);

    return std::string_view{p.parent_path().string()};
}
