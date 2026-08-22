#include <MazeBuilder/stringify_create_state.h>

#include <MazeBuilder/barriers.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/io_utils.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/runtime_stack.h>

#include <string>

using namespace mazes;

stringify_create_state::stringify_create_state(const runtime_app::context& ctx, runtime_stack* rs)
    : write_to_output_state(ctx, rs)
{
}

bool stringify_create_state::write_output(const std::string& output_target, const std::string& output_content) noexcept
{
    return io_utils::write_file(output_target, output_content);
}

std::string_view stringify_create_state::create(const configurator& config, [[maybe_unused]] randomizer& rng) noexcept
{
    auto render_ascii_maze = [](const grid_interface& grid, const grid_operations& grid_ops,
        const unsigned int rows, const unsigned int cols) -> std::string
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
            for (unsigned int r = 0; r < rows; ++r)
            {
                for (unsigned int c = 0; c < cols; ++c)
                {
                    if (const auto cp = grid_ops.search(static_cast<int>(r * cols + c)); cp)
                    {
                        cell_width = std::max(cell_width, grid.contents_of(cp).length());
                    }
                }
            }

            constexpr char H_WALL = static_cast<char>(Barrier::HORIZONTAL);
            constexpr char V_WALL = static_cast<char>(Barrier::VERTICAL);
            constexpr char CORNER = static_cast<char>(Barrier::CORNER);

            const std::string horizontal_wall(cell_width, H_WALL);
            const std::string horizontal_gap(cell_width, ' ');

            std::string result;
            result.reserve((rows + 1u) * (cols * (cell_width + 1u) + 2u));

            result += CORNER;
            for (unsigned int c = 0; c < cols; ++c)
            {
                result += horizontal_wall;
                result += CORNER;
            }
            result += '\n';

            for (unsigned int r = 0; r < rows; ++r)
            {
                std::string top = std::string(1, V_WALL);
                std::string bottom = std::string(1, CORNER);
                for (unsigned int c = 0; c < cols; ++c)
                {
                    const auto cp = grid_ops.search(static_cast<int>(r * cols + c));
                    const auto e = cp ? grid_ops.get_east(cp) : nullptr;
                    const auto s = cp ? grid_ops.get_south(cp) : nullptr;

                    top += cp ? pad_content(grid.contents_of(cp), cell_width) : pad_content(" ", cell_width);
                    top += (cp && e && cp->is_linked(e)) ? ' ' : V_WALL;
                    bottom += (cp && s && cp->is_linked(s)) ? horizontal_gap : horizontal_wall;
                    bottom += CORNER;
                }

                result += top;
                result += '\n';
                result += bottom;
                result += '\n';
            }

            return result;
        };

    const auto grid_id = config.distances() ? grid_identifier::DISTANCE : grid_identifier::BASIC;
    try
    {
        auto& grid_ref = grid_mapper->get(grid_id);
        m_result = render_ascii_maze(std::cref(grid_ref), std::cref(grid_ref.operations()), config.rows(), config.columns());
        return m_result;
    } catch (...)
    {
        return {};
    }
}
