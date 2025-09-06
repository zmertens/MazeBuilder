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

    // Safety check for excessively large grids to prevent performance issues
    const size_t total_cells = static_cast<size_t>(rows) * static_cast<size_t>(columns) * static_cast<size_t>(levels);
    static constexpr auto MAX_REASONABLE_CELLS = configurator::MAX_COLUMNS * configurator::MAX_ROWS * configurator::MAX_LEVELS + 1u;

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
        result = string_utils::concat(result, "---+");
    }
    result = string_utils::concat(result, "\n");

    // For each row
    for (auto r = 0u; r < rows; ++r)
    {
        std::string top_line = "|";
        std::string bottom_line = "+";

        // For each column
        for (auto c = 0u; c < columns; ++c)
        {
            // Get cell on-demand (will be created lazily if needed)
            if (auto cell_ptr = ops.search(r * columns + c); cell_ptr != nullptr)
            {
                std::string content = g->contents_of(cell_ptr);

                // Pad content to 3 characters
                static constexpr auto pad_content = [](std::string &str)
                {
                    while (str.length() < 5)
                    {
                        str = " " + str;
                    }
                };
                pad_content(content);

                top_line += content;

                static constexpr auto MAX_LINKS = 100u;
                // Safety check: limit the number of links to check
                auto link_count = 0;

                // East wall
                if (auto east_neighbor = ops.get_east(cell_ptr); east_neighbor != nullptr)
                {
                    bool linked_east = false;

                    auto links = cell_ptr->get_links();

                    for (const auto &[linked_cell, is_linked] : links)
                    {
                        if (++link_count > MAX_LINKS)
                        {
                            ops.set_str("Grid too large to stringify reasonably.");

                            return false;
                        }

                        if (is_linked)
                        {
                            auto locked = linked_cell;
                            if (locked && locked->get_index() == east_neighbor->get_index())
                            {
                                linked_east = true;
                                break;
                            }
                        }
                    }

                    top_line += linked_east ? " " : "|";
                }

                link_count = 0u;

                // South wall
                if (auto south_neighbor = ops.get_south(cell_ptr); south_neighbor != nullptr)
                {
                    bool linked_south = false;

                    auto links = cell_ptr->get_links();

                    for (const auto &[linked_cell, is_linked] : links)
                    {
                        if (++link_count > MAX_LINKS)
                        {
                            ops.set_str("Grid too large to stringify reasonably.");

                            return false;
                        }

                        if (is_linked)
                        {
                            auto locked = linked_cell;
                            if (locked && locked->get_index() == south_neighbor->get_index())
                            {
                                linked_south = true;
                                break;
                            }
                        }
                    }

                    bottom_line += linked_south ? "   " : "---";
                    bottom_line += "+";
                }
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
