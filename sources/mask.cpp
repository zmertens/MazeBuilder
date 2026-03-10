#include <MazeBuilder/mask.h>

#include <fstream>
#include <stdexcept>
#include <string>

using namespace mazes;

mask::mask(unsigned int rows, unsigned int columns)
    : m_rows(rows), m_columns(columns),
      m_bits(rows, std::vector<bool>(columns, true))
{
}

bool mask::operator()(unsigned int row, unsigned int column) const noexcept
{
    if (row >= m_rows || column >= m_columns)
    {
        return false;
    }
    return m_bits[row][column];
}

void mask::set(unsigned int row, unsigned int column, bool is_on) noexcept
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

std::pair<unsigned int, unsigned int> mask::random_location(randomizer &rng) const noexcept
{
    while (true)
    {
        const auto row = static_cast<unsigned int>(rng.get_int(0, static_cast<int>(m_rows) - 1));
        const auto col = static_cast<unsigned int>(rng.get_int(0, static_cast<int>(m_columns) - 1));
        if (m_bits[row][col])
        {
            return {row, col};
        }
    }
}

mask mask::from_txt(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open mask file: " + filename);
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
    {
        // Strip trailing carriage return and spaces (std::getline already strips \n)
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
        {
            line.pop_back();
        }
        if (!line.empty())
        {
            lines.push_back(line);
        }
    }

    if (lines.empty())
    {
        throw std::runtime_error("Mask file is empty: " + filename);
    }

    const auto rows = static_cast<unsigned int>(lines.size());

    // Use the maximum line length as the column count for potentially ragged input
    std::size_t max_cols = 0;
    for (const auto &l : lines)
    {
        if (l.size() > max_cols) max_cols = l.size();
    }
    const auto columns = static_cast<unsigned int>(max_cols);

    // Cells in shorter rows that are missing default to available (true)
    mask m(rows, columns);
    for (unsigned int row = 0; row < rows; ++row)
    {
        for (unsigned int col = 0; col < static_cast<unsigned int>(lines[row].size()); ++col)
        {
            m.set(row, col, lines[row][col] != 'X');
        }
    }

    return m;
}
