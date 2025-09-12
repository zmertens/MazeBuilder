#include <MazeBuilder/stringify.h>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/string_utils.h>

using namespace mazes;

/// @brief Provide a string representation of the grid
/// @param g
/// @param rng
/// @return
bool stringify::run(grid_interface *g, [[maybe_unused]] randomizer &rng) const noexcept
{

    if (!g)
    {

        return false;
    }

    std::string result{};
    result.reserve(1024);

    auto &&ops = g->operations();

    auto [rows, columns, levels] = ops.get_dimensions();

    static constexpr auto MAX_REASONABLE_CELLS = configurator::MAX_COLUMNS * configurator::MAX_ROWS * configurator::MAX_LEVELS + 1u;

    const size_t total_cells = static_cast<size_t>(rows) * static_cast<size_t>(columns) * static_cast<size_t>(levels);

    if (total_cells > MAX_REASONABLE_CELLS)
    {
        ops.set_str("Grid too large to stringify reasonably.");

        return false;
    }

    // Generate ASCII representation
    // Top border
    result = string_utils::concat(result, "+");
    for (auto c = 0u; c < columns; ++c)
    {
        result = string_utils::concat(result, "-----+");
    }
    result = string_utils::concat(result, "\n");

    // For each row
    for (auto r = 0u; r < rows; ++r)
    {
        std::string top_line = "|";
        std::string bottom_line = "+";

        for (auto c = 0u; c < columns; ++c)
        {
            // Get cell on-demand (will be created lazily if needed)
            if (auto cell_ptr = ops.search(r * columns + c); cell_ptr != nullptr)
            {
                std::string content = g->contents_of(cell_ptr);

                static constexpr auto pad_content = [](std::string &str)
                {
                    static constexpr auto MAX_CONTENT_LENGTH = 5u;

                    while (str.length() < MAX_CONTENT_LENGTH)
                    {
                        str = " " + str;
                    }
                };
                pad_content(content);

                top_line += content;

                // East wall - FIXED: Always add a wall, check if it should be open
                if (auto east_neighbor = ops.get_east(cell_ptr); east_neighbor != nullptr)
                {
                    bool linked_east = false;

                    for (const auto &[linked_cell, is_linked] : cell_ptr->get_links())
                    {
                            if (is_linked && linked_cell->get_index() == east_neighbor->get_index())
                            {
                                linked_east = true;
                                break;
                            }
                     
                    }

                    top_line += linked_east ? " " : "|";
                }
                else
                {
                    // No east neighbor (rightmost column) - always add wall
                    top_line += "|";
                }

                // South wall - FIXED: Always add bottom border for every cell
                if (auto south_neighbor = ops.get_south(cell_ptr); south_neighbor != nullptr)
                {
                    bool linked_south = false;

                    for (const auto &[linked_cell, is_linked] : cell_ptr->get_links())
                    {
                            if (is_linked && linked_cell->get_index() == south_neighbor->get_index())
                            {
                                linked_south = true;
                                break;
                            }

                    }

                    bottom_line += linked_south ? "     " : "-----";
                }
                else
                {
                    // No south neighbor (bottom row) - always add bottom wall
                    bottom_line += "-----";
                }

                bottom_line += "+";
            }
            else
            {
                // Handle case where cell doesn't exist
                top_line += "     |";
                bottom_line += "-----+";
            }
        }

        result = string_utils::concat(result, top_line);
        result = string_utils::concat(result, "\n");
        result = string_utils::concat(result, bottom_line);
        result = string_utils::concat(result, "\n");
    }

    ops.set_str(result);

    return true;
} // run
