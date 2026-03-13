#include <MazeBuilder/io_utils.h>

#include <filesystem>
#include <fstream>

using namespace mazes;

// delimiter = "\n"
bool io_utils::write(std::ostream &oss, const std::string &data, std::string_view delimiter) const noexcept
{
    oss << data << delimiter;

    return oss.good();
}

/// @brief Write to a conventional file
/// @param filename
/// @param data
/// @return
bool io_utils::write_file(const std::string &filename, const std::string &data) const noexcept
{
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
