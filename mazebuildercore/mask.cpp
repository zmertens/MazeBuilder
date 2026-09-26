#include <MazeBuilder/mask.h>
#include <MazeBuilder/string_utils.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
    namespace fs = std::filesystem;

    void strip_wrapping_pair(std::string& text, const char open, const char close)
    {
        if (text.size() >= 2 && text.front() == open && text.back() == close)
        {
            text = text.substr(1, text.size() - 2);
        }
    }

    std::optional<fs::path> resolve_mask_file_path(const std::string_view filename)
    {
        if (filename.empty())
        {
            return std::nullopt;
        }

        fs::path resolved{ filename };
        if (fs::exists(resolved))
        {
            return resolved;
        }

        static constexpr const char* fallback_dirs[] = { "tests", "scripts" };
        for (const char* dir : fallback_dirs)
        {
            fs::path candidate = fs::path(dir) / std::string(filename);
            if (fs::exists(candidate))
            {
                return candidate;
            }
        }

        return std::nullopt;
    }

    std::vector<std::string> read_mask_lines(std::istream& input)
    {
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(input, line))
        {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            {
                line.pop_back();
            }

            if (!line.empty())
            {
                lines.push_back(line);
            }
        }
        return lines;
    }

    mazes::mask build_mask_from_lines(const std::vector<std::string>& lines, const std::string& source_name)
    {
        if (lines.empty())
        {
            throw std::runtime_error(source_name.empty() ? "Mask input is empty" : source_name);
        }

        const auto rows = static_cast<unsigned int>(lines.size());

        std::size_t max_cols = 0;
        for (const auto& l : lines)
        {
            if (l.size() > max_cols)
            {
                max_cols = l.size();
            }
        }

        const auto columns = static_cast<unsigned int>(max_cols);
        mazes::mask m(rows, columns);
        for (unsigned int row = 0; row < rows; ++row)
        {
            for (unsigned int col = 0; col < static_cast<unsigned int>(lines[row].size()); ++col)
            {
                m.set(row, col, lines[row][col] != 'X');
            }
        }

        return m;
    }

    bool looks_like_inline_mask(std::string_view source)
    {
        if (source.empty())
        {
            return false;
        }

        if ((source.front() == '`' && source.back() == '`') ||
            (source.front() == '"' && source.back() == '"') ||
            (source.front() == '\'' && source.back() == '\'') ||
            source.find('\n') != std::string_view::npos ||
            source.find('\r') != std::string_view::npos ||
            source.find("\\n") != std::string_view::npos ||
            source.find("\\r") != std::string_view::npos)
        {
            return true;
        }
        return false;
    }
}

using namespace mazes;

mask::mask(const unsigned int rows, const unsigned int columns)
    : m_rows(rows), m_columns(columns),
    m_bits(rows, std::vector<bool>(columns, true))
{
}

bool mask::operator()(const unsigned int row, const unsigned int column) const noexcept
{
    if (row >= m_rows || column >= m_columns)
    {
        return false;
    }
    return m_bits[row][column];
}

void mask::set(const unsigned int row, const unsigned int column, const bool is_on) noexcept
{
    if (row < m_rows && column < m_columns)
    {
        m_bits[row][column] = is_on;
    }
}

int mask::count() const noexcept
{
    int total = 0;
    for (unsigned int row = 0; row < m_rows; ++row)
    {
        for (unsigned int col = 0; col < m_columns; ++col)
        {
            if (m_bits[row][col])
            {
                ++total;
            }
        }
    }
    return total;
}

std::pair<unsigned int, unsigned int> mask::random_location(randomizer& rng) const noexcept
{
    while (true)
    {
        const auto row = static_cast<unsigned int>(rng.get_int(0, static_cast<int>(m_rows) - 1));
        if (const auto col = static_cast<unsigned int>(rng.get_int(0, static_cast<int>(m_columns) - 1)); m_bits[row][col])
        {
            return { row, col };
        }
    }
}

mask mask::from_txt(const std::string& filename)
{
    const auto resolved = resolve_mask_file_path(filename);
    if (!resolved.has_value())
    {
        throw std::runtime_error("Could not open mask file: " + filename);
    }

    std::ifstream file(*resolved);
    const auto lines = read_mask_lines(file);
    return build_mask_from_lines(lines, "Mask file is empty: " + filename);
}

mask mask::from_string(std::string_view text)
{
    if (text.empty())
    {
        throw std::runtime_error("Mask string is empty");
    }

    std::string normalized{ text };
    strip_wrapping_pair(normalized, '"', '"');
    strip_wrapping_pair(normalized, '\'', '\'');
    strip_wrapping_pair(normalized, '`', '`');

    normalized = string_utils::replace_all(normalized, "\\r\\n", "\n");
    normalized = string_utils::replace_all(normalized, "\\n", "\n");
    normalized = string_utils::replace_all(normalized, "\\r", "\n");

    std::istringstream input{ normalized };
    const auto lines = read_mask_lines(input);
    return build_mask_from_lines(lines, "Mask string is empty");
}

mask mask::from_source(const std::string_view source)
{
    if (source.empty())
    {
        throw std::runtime_error("Mask input is empty");
    }

    if (const auto resolved = resolve_mask_file_path(source); resolved.has_value())
    {
        return from_txt(std::string{ source });
    }

    if (looks_like_inline_mask(source))
    {
        return from_string(source);
    }

    throw std::runtime_error("Could not open mask file: " + std::string(source));
}
