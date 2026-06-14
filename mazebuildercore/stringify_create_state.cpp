#include <MazeBuilder/stringify_create_state.h>

#include <MazeBuilder/args.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/runtime_stack.h>

using namespace mazes;

stringify_create_state::stringify_create_state(const runtime_app::context &ctx, runtime_stack *stack)
: state(ctx, stack), grid_mapper{ctx.get_grid_manager()}, processed_text_mapper{ctx.get_text_manager()}
{

}

void stringify_create_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool stringify_create_state::update([[maybe_unused]] const std::optional<args> &args, [[maybe_unused]] double delta_time) noexcept
{
    // Implementation of the update function
    return true;
}

std::string_view stringify_create_state::create([[maybe_unused]] algo a, unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept
{
    auto sum = rows + cols + levels;
    [[maybe_unused]] auto sum1 = rng(0, sum);
    return std::string_view{"Objectify maze creation not yet implemented"};
}
    //   if (!g)
    //     {
    //         return false;
    //     }

    //     std::string result{};
    //     result.reserve(1024);

    //     auto &&ops = g->operations();

    //     auto [rows, columns, levels] = ops.get_dimensions();

    //     static constexpr auto MAX_REASONABLE_CELLS = configurator::MAX_COLUMNS * configurator::MAX_ROWS * configurator::MAX_LEVELS + 1u;

    //     if (const size_t total_cells = static_cast<size_t>(rows) * static_cast<size_t>(columns) * static_cast<size_t>(levels); total_cells > MAX_REASONABLE_CELLS)
    //     {
    //         ops.set_str("Grid too large to stringify reasonably.");

    //         return false;
    //     }

    //     // Generate ASCII representation
    //     // Top border
    //     result = string_utils::concat(result, "+");
    //     for (auto c = 0u; c < columns; ++c)
    //     {
    //         result = string_utils::concat(result, "-----+");
    //     }
    //     result = string_utils::concat(result, "\n");

    //     // For each row
    //     for (auto r = 0u; r < rows; ++r)
    //     {
    //         std::string top_line = "|";
    //         std::string bottom_line = "+";

    //         for (auto c = 0u; c < columns; ++c)
    //         {
    //             // Get cell on-demand (will be created lazily if needed)
    //             if (auto cell_ptr = ops.search(r * columns + c); cell_ptr != nullptr)
    //             {
    //                 std::string content = g->contents_of(cell_ptr);

    //                 static constexpr auto pad_content = [](std::string &str)
    //                 {
    //                     static constexpr auto MAX_CONTENT_LENGTH = 5u;

    //                     while (str.length() < MAX_CONTENT_LENGTH)
    //                     {
    //                         str = " " + str;
    //                     }
    //                 };

    //                 pad_content(content);

    //                 top_line = string_utils::concat(top_line, content);

    //                 // East wall - FIXED: Always add a wall, check if it should be open
    //                 if (auto east_neighbor = ops.get_east(cell_ptr); east_neighbor != nullptr)
    //                 {
    //                     bool linked_east = false;

    //                     for (const auto &[linked_cell, is_linked] : cell_ptr->get_links())
    //                     {
    //                         if (is_linked && linked_cell->get_index() == east_neighbor->get_index())
    //                         {
    //                             linked_east = true;
    //                             break;
    //                         }
    //                     }

    //                     top_line += linked_east ? " " : "|";
    //                 }
    //                 else
    //                 {
    //                     // No east neighbor (rightmost column) - always add wall
    //                     top_line += "|";
    //                 }

    //                 // South wall - FIXED: Always add bottom border for every cell
    //                 if (auto south_neighbor = ops.get_south(cell_ptr); south_neighbor != nullptr)
    //                 {
    //                     bool linked_south = false;

    //                     for (const auto &[linked_cell, is_linked] : cell_ptr->get_links())
    //                     {
    //                         if (is_linked && linked_cell->get_index() == south_neighbor->get_index())
    //                         {
    //                             linked_south = true;
    //                             break;
    //                         }
    //                     }

    //                     bottom_line += linked_south ? "     " : "-----";
    //                 }
    //                 else
    //                 {
    //                     // No south neighbor (bottom row) - always add bottom wall
    //                     bottom_line += "-----";
    //                 }

    //                 bottom_line += "+";
    //             }
    //             else
    //             {
    //                 // Handle case where cell doesn't exist
    //                 top_line += "     |";
    //                 bottom_line += "-----+";
    //             }
    //         }

    //         result = string_utils::concat(result, top_line);
    //         result = string_utils::concat(result, "\n");
    //         result = string_utils::concat(result, bottom_line);
    //         result = string_utils::concat(result, "\n");
    //     }

    //     ops.set_str(result);

    //     return true;