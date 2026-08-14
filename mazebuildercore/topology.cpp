#include <MazeBuilder/topology.h>

#include <algorithm>
#include <cstddef>

using namespace mazes;

const cell_walls* topology::at(const unsigned int row, const unsigned int col) const noexcept
{
    if (row >= rows || col >= columns)
    {
        return nullptr;
    }

    const auto idx = static_cast<std::size_t>(row) * static_cast<std::size_t>(columns) + static_cast<std::size_t>(col);
    return &cells[idx];
}

topology topology::parse(const std::string_view txt) noexcept
{
    if (txt.empty())
    {
        return {};
    }

    std::vector<std::string_view> lines;
    lines.reserve(static_cast<std::size_t>(std::count(txt.begin(), txt.end(), '\n')) + 1u);

    std::size_t start = 0u;
    while (start <= txt.size())
    {
        const std::size_t end = txt.find('\n', start);
        std::string_view line = (end == std::string_view::npos)
                                    ? txt.substr(start)
                                    : txt.substr(start, end - start);

        if (!line.empty() && line.back() == '\r')
        {
            line.remove_suffix(1u);
        }

        if (!line.empty())
        {
            lines.push_back(line);
        }

        if (end == std::string_view::npos)
        {
            break;
        }
        start = end + 1u;
    }

    if (lines.empty() || lines.at(0).size() < 3u)
    {
        return {};
    }

    const std::string_view top_border = lines.front();
    std::vector<std::size_t> plus_positions;
    plus_positions.reserve(top_border.size());
    for (std::size_t i = 0u; i < top_border.size(); ++i)
    {
        if (top_border[i] == '+')
        {
            plus_positions.push_back(i);
        }
    }

    if (plus_positions.size() < 2u)
    {
        return {};
    }

    const auto rows_count = static_cast<unsigned int>((lines.size() - 1u) / 2u);
    const auto cols_count = static_cast<unsigned int>(plus_positions.size() - 1u);

    topology out{};
    out.rows = rows_count;
    out.columns = cols_count;
    out.cells.resize(static_cast<std::size_t>(out.rows) * static_cast<std::size_t>(out.columns));

    auto has_horizontal_wall = [](const std::string_view border, const std::size_t from, const std::size_t to) -> bool
    {
        if (from >= border.size() || to > border.size() || from >= to)
        {
            return false;
        }

        for (std::size_t i = from; i < to; ++i)
        {
            if (border[i] == '-')
            {
                return true;
            }
        }

        return false;
    };

    for (unsigned int row = 0u; row < out.rows; ++row)
    {
        const std::size_t top_line_index = 1u + static_cast<std::size_t>(row) * 2u;
        const std::size_t bottom_line_index = top_line_index + 1u;
        const std::size_t north_border_index = static_cast<std::size_t>(row) * 2u;

        if (bottom_line_index >= lines.size() || north_border_index >= lines.size())
        {
            break;
        }

        const std::string_view top_line = lines[top_line_index];
        const std::string_view bottom_line = lines[bottom_line_index];
        const std::string_view north_border = lines[north_border_index];

        for (unsigned int col = 0u; col < out.columns; ++col)
        {
            const std::size_t left = plus_positions[col];
            const std::size_t right = plus_positions[col + 1u];

            cell_walls cell_data{};
            cell_data.set_north(has_horizontal_wall(north_border, left + 1u, right));
            cell_data.set_south(has_horizontal_wall(bottom_line, left + 1u, right));

            const std::size_t west_idx = left;
            const std::size_t east_idx = right;
            cell_data.set_west((west_idx < top_line.size()) ? (top_line[west_idx] == '|') : true);
            cell_data.set_east((east_idx < top_line.size()) ? (top_line[east_idx] == '|') : true);

            const auto idx = static_cast<std::size_t>(row) * static_cast<std::size_t>(out.columns) + static_cast<std::size_t>(col);
            out.cells[idx] = cell_data;
        }
    }

    return out;
}
