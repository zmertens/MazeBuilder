#include <MazeBuilder/string_utils.h>

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <sstream>

#include <fmt/format.h>

#include <MazeBuilder/output_formats.h>

using namespace mazes;

std::string string_utils::concat(const std::string& a, const std::string& b) noexcept
{
    return fmt::format("{}{}", a, b);
}

bool string_utils::contains(const std::string& str, const std::string& substr) noexcept
{
    return str.find(substr) != std::string::npos;
}

bool string_utils::ends_with(const std::string& str, const std::string& suffix) noexcept
{
    return std::string_view{ str }.substr(str.size() - suffix.size()) == suffix;
}

std::string string_utils::file_extension(std::string_view filename) noexcept
{
    if (filename.empty())
    {
        return {};
    }

    const std::string name{ filename };
    if (auto _stdout{to_sv_from_output_format(output_format::STDOUT)}; name == _stdout)
    {
        return std::string{_stdout};
    }

    const auto ext = std::filesystem::path{ name }.extension().string();
    if (ext.empty() || ext == ".")
    {
        return {};
    }

    std::string lower{};
    lower.reserve(ext.size());
    for (const unsigned char ch : ext)
    {
        lower.push_back(static_cast<char>(std::tolower(ch)));
    }
    return lower.substr(1);
}
