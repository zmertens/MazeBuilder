#include <MazeBuilder/stringify_create_state.h>

#include <MazeBuilder/barriers.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/io_utils.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/runtime_stack.h>

#include <algorithm>
#include <string>
#include <vector>

using namespace mazes;

stringify_create_state::stringify_create_state(const runtime_app::context& ctx, runtime_stack* rs)
    : write_to_output_state(ctx, rs)
{
}

bool stringify_create_state::write_output(const std::string& output_target, const std::string& output_content) noexcept
{
    return io_utils::write_file(output_target, output_content);
}

std::any stringify_create_state::create(const args &a, randomizer &rng) noexcept
{
    const auto config = make_config(a);

    auto render_ascii_maze = [](const grid_interface& grid, const grid_operations& grid_ops,
        const unsigned int rows, const unsigned int cols, const unsigned int levels) -> std::string
        {
            auto pad_content = [](std::string content, const std::size_t cell_width) -> std::string
                {
                    if (content.length() >= cell_width)
                    {
                        return content;
                    }

                    const auto remaining = cell_width - content.length();
                    const auto left_pad = remaining / 2u;
                    const auto right_pad = remaining - left_pad;

                    content.insert(content.begin(), left_pad, ' ');
                    content.append(right_pad, ' ');

                    return content;
                };

            // Keep the classic compact maze by default, but widen when cell text requires it.
            std::size_t cell_width = 3u;
            for (const auto& cp : grid_ops.cells())
            {
                if (cp)
                {
                    cell_width = std::max(cell_width, grid.contents_of(cp).length());
                }
            }

            constexpr char H_WALL = static_cast<char>(Barrier::HORIZONTAL);
            constexpr char V_WALL = static_cast<char>(Barrier::VERTICAL);
            constexpr char CORNER = static_cast<char>(Barrier::CORNER);

            const std::string horizontal_wall(cell_width, H_WALL);
            const std::string horizontal_gap(cell_width, ' ');

            auto render_level = [&](const unsigned int level) -> std::vector<std::string>
                {
                    const auto base = static_cast<int>(level * rows * cols);
                    std::vector<std::string> lines;
                    lines.reserve(rows * 2u + 1u);

                    std::string border(1, CORNER);
                    for (unsigned int c = 0; c < cols; ++c)
                    {
                        border += horizontal_wall;
                        border += CORNER;
                    }
                    lines.push_back(std::move(border));

                    for (unsigned int r = 0; r < rows; ++r)
                    {
                        std::string top = std::string(1, V_WALL);
                        std::string bottom = std::string(1, CORNER);
                        for (unsigned int c = 0; c < cols; ++c)
                        {
                            const auto cp = grid_ops.search(base + static_cast<int>(r * cols + c));
                            const auto e = cp ? grid_ops.get_east(cp) : nullptr;
                            const auto s = cp ? grid_ops.get_south(cp) : nullptr;

                            top += cp ? pad_content(grid.contents_of(cp), cell_width) : pad_content(" ", cell_width);
                            top += (cp && e && cp->is_linked(e)) ? ' ' : V_WALL;
                            bottom += (cp && s && cp->is_linked(s)) ? horizontal_gap : horizontal_wall;
                            bottom += CORNER;
                        }
                        lines.push_back(std::move(top));
                        lines.push_back(std::move(bottom));
                    }
                    return lines;
                };

            // Discrete mazes are tiled left to right, wrapping after `cols` mazes
            const unsigned int per_row = std::max(1u, std::min(cols, levels));
            const std::string tile_gap(2u, ' ');

            std::string result;
            for (unsigned int first = 0; first < levels; first += per_row)
            {
                if (first > 0)
                {
                    result += '\n';
                }

                const auto last = std::min(first + per_row, levels);
                std::vector<std::vector<std::string>> tiles;
                for (unsigned int lv = first; lv < last; ++lv)
                {
                    tiles.push_back(render_level(lv));
                }

                for (std::size_t line = 0; line < tiles.front().size(); ++line)
                {
                    for (std::size_t t = 0; t < tiles.size(); ++t)
                    {
                        if (t > 0)
                        {
                            result += tile_gap;
                        }
                        result += tiles[t][line];
                    }
                    result += '\n';
                }
            }

            return result;
        };

    const auto grid_id = config.distances() ? grid_identifier::DISTANCE : grid_identifier::BASIC;
    try
    {
        auto& grid_ref = grid_mapper->get(grid_id);
        m_output = render_ascii_maze(std::cref(grid_ref), std::cref(grid_ref.operations()), config.rows(), config.columns(), config.levels());
        return m_output;
    } catch (...)
    {
        return {};
    }
}
