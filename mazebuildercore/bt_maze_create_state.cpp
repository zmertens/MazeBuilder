#include <MazeBuilder/bt_maze_create_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/runtime_stack.h>

#include <fmt/format.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/resource_management.h>

#include <string>
#include <vector>

using namespace mazes;

bt_maze_create_state::bt_maze_create_state(const runtime_app::context &ctx, runtime_stack *stack)
    : state(ctx, stack),
      grid_mapper{ctx.get_grid_manager()},
      processed_text_mapper{ctx.get_text_manager()}
{

}

std::string_view bt_maze_create_state::create(algo a, unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept
{
    if (a != algo::BINARY_TREE)
    {
        return {};
    }

    return create_bt_maze(rows, cols, levels, std::ref(rng));   
}

void bt_maze_create_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool bt_maze_create_state::update([[maybe_unused]] const std::optional<args> &args, [[maybe_unused]] double delta_time) noexcept
{
    if (!args.has_value() || !grid_mapper || !processed_text_mapper)
    {
        request_stack_pop();
        return false;
    }

    unsigned int rows   = configurator::MAX_ROWS;
    unsigned int cols   = configurator::MAX_COLUMNS;
    unsigned int levels = 1u;

    if (auto parsed = args->get(); parsed.has_value())
    {
        if (auto it = parsed->find(mazes::args::ROW_WORD_STR);    it != parsed->end())
            try { rows   = static_cast<unsigned int>(std::stoul(it->second)); } catch (...) {}
        if (auto it = parsed->find(mazes::args::COLUMN_WORD_STR); it != parsed->end())
            try { cols   = static_cast<unsigned int>(std::stoul(it->second)); } catch (...) {}
        if (auto it = parsed->find(mazes::args::LEVEL_WORD_STR);  it != parsed->end())
            try { levels = static_cast<unsigned int>(std::stoul(it->second)); } catch (...) {}
    }

    try { grid_mapper->get(grid_identifier::BASIC).operations().resize(rows, cols, levels); }
    catch (...) { request_stack_pop(); return false; }

    randomizer fallback_rng{};
    randomizer *rng_ptr = &fallback_rng;
    if (auto opt = get_context().get_rng(); opt.has_value()) rng_ptr = &opt->get();

    auto result = create(algo::BINARY_TREE, rows, cols, levels, *rng_ptr);
    if (!result.empty())
    {
        try
        {
            auto &txt = processed_text_mapper->get(processed_text_identifier::FINISHED);
            txt.set_dirty(std::string{result});
            txt.set_processed(std::string{result});
        }
        catch (...) {}
    }

    request_stack_pop();
    return false;
}

std::string_view bt_maze_create_state::create_bt_maze(unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept
{
    if (!grid_mapper) return {};
    try
    {
        auto &grid_ops = grid_mapper->get(grid_identifier::BASIC).operations();

        for (unsigned int lv = 0; lv < levels; ++lv)
        {
            for (unsigned int row = 0; row < rows; ++row)
            {
                for (unsigned int col = 0; col < cols; ++col)
                {
                    auto c = grid_ops.search(static_cast<int>(lv * rows * cols + row * cols + col));
                    if (!c) continue;
                    std::vector<std::shared_ptr<cell>> cands;
                    if (auto n = grid_ops.get_north(c)) cands.push_back(n);
                    if (auto e = grid_ops.get_east(c))  cands.push_back(e);
                    if (!cands.empty())
                        lab::link(c, cands.at(static_cast<size_t>(rng(0, static_cast<int>(cands.size()) - 1))), true);
                }
            }
        }

        m_result.clear();
        m_result += "+";
        for (unsigned int c = 0; c < cols; ++c) m_result += "---+";
        m_result += "\n";

        for (unsigned int r = 0; r < rows; ++r)
        {
            std::string top = "|", bot = "+";
            for (unsigned int c = 0; c < cols; ++c)
            {
                auto cp = grid_ops.search(static_cast<int>(r * cols + c));
                auto e  = cp ? grid_ops.get_east(cp)  : nullptr;
                auto s  = cp ? grid_ops.get_south(cp) : nullptr;
                top += "   ";
                top += (cp && e && cp->is_linked(e)) ? " " : "|";
                bot += (cp && s && cp->is_linked(s)) ? "   +" : "---+";
            }
            m_result += top + "\n" + bot + "\n";
        }

        grid_ops.set_str(m_result);
        return m_result;
    }
    catch (...) { return {}; }
}
