#include <MazeBuilder/dfs_maze_create_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/randomizer.h>

#include <fmt/format.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/resource_management.h>
#include <MazeBuilder/runtime_stack.h>

#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

using namespace mazes;

dfs_maze_create_state::dfs_maze_create_state(const runtime_app::context &ctx, runtime_stack *stack)
: state(ctx, stack), grid_mapper{ctx.get_grid_manager()}, processed_text_mapper{ctx.get_text_manager()}
{

}

std::string_view dfs_maze_create_state::create(algo a, unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept
{
    if (a != algo::DFS)
    {
        return std::string_view{"Unsupported algorithm"};
    }

    return create_dfs_maze(rows, cols, levels, std::ref(rng));   
}

void dfs_maze_create_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool dfs_maze_create_state::update([[maybe_unused]] const std::optional<args> &args, [[maybe_unused]] double delta_time) noexcept
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

    auto result = create(algo::DFS, rows, cols, levels, *rng_ptr);
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

std::string_view dfs_maze_create_state::create_dfs_maze(unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept
{
    [[maybe_unused]] auto _levels = levels; // DFS operates on level 0; multi-level support is future work
    if (!grid_mapper) return {};
    try
    {
        auto &grid_ops = grid_mapper->get(grid_identifier::BASIC).operations();

        // Start in the first cell of level 0
        const int level0_max = static_cast<int>(rows * cols) - 1;
        auto start = grid_ops.search(rng(0, level0_max));
        if (!start) return {};

        std::stack<std::shared_ptr<cell>> stk;
        std::unordered_set<int> visited;
        stk.push(start);
        visited.insert(start->get_index());

        while (!stk.empty())
        {
            auto current = stk.top();
            auto all_neighbors = grid_ops.get_neighbors(current);

            std::vector<std::shared_ptr<cell>> unvisited;
            for (auto &n : all_neighbors)
                if (n && visited.find(n->get_index()) == visited.end()) unvisited.push_back(n);

            if (unvisited.empty())
            {
                stk.pop();
            }
            else
            {
                auto &chosen = unvisited.at(static_cast<size_t>(rng(0, static_cast<int>(unvisited.size()) - 1)));
                lab::link(current, chosen, true);
                visited.insert(chosen->get_index());
                stk.push(chosen);
            }
        }

        // Stringify
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
