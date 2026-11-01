#include <MazeBuilder/lab.h>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/randomizer.h>

#include <memory>
#include <span>
#include <stack>
#include <unordered_set>
#include <vector>

using namespace mazes;

/// @brief Create a bidirectional or unidirectional link between two cells
/// @param c1 First cell to link
/// @param c2 Second cell to link
/// @param bidi If true, creates bidirectional link; if false, only c1 links to c2
void lab::link(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, const bool bidi) noexcept
{
    if (!c1 || !c2)
    {
        return;
    }

    // Add c2 to c1's links
    c1->add_link(c2);

    // If bidirectional, add c1 to c2's links
    if (bidi)
    {
        c2->add_link(c1);
    }
}

/// @brief Remove link between two cells
/// @param c1 First cell to unlink
/// @param c2 Second cell to unlink
/// @param bidi If true, removes bidirectional link; if false, only removes c1's link to c2
void lab::unlink(const std::shared_ptr<cell>& c1, const std::shared_ptr<cell>& c2, const bool bidi) noexcept
{
    if (!c1 || !c2)
    {
        return;
    }

    // Remove c2 from c1's links
    c1->remove_link(c2);

    // If bidirectional, remove c1 from c2's links
    if (bidi)
    {
        c2->remove_link(c1);
    }
}

namespace
{
    struct dims
    {
        int rows;
        int cols;
        int levels;
        int level_size() const noexcept { return rows * cols; }
    };

    dims dims_of(const grid_operations &ops) noexcept
    {
        const auto [rows, cols, levels] = ops.get_dimensions();
        return {rows, cols, levels};
    }

    bool is_open(const lab::cell_filter &open, int index) noexcept
    {
        return !open || open(index);
    }

    bool is_open(const grid_operations &ops, const lab::cell_filter &open, const std::shared_ptr<cell> &c) noexcept
    {
        return c && is_open(open, ops.index_of(c));
    }

    // A random accepted cell of one level, scanning on from a random pick when it is rejected
    std::shared_ptr<cell> pick_start(const grid_operations &ops, const lab::cell_filter &open, int offset, int level_size,
                                     randomizer &rng)
    {
        const int first = rng(0, level_size - 1);
        for (int i = 0; i < level_size; ++i)
        {
            const int index = offset + (first + i) % level_size;
            if (!is_open(open, index))
            {
                continue;
            }
            if (auto c = ops.search(index))
            {
                return c;
            }
        }
        return nullptr;
    }
} // namespace

bool lab::binary_tree(grid_operations &ops, randomizer &rng, const lab::cell_filter &open)
{
    const auto d = dims_of(ops);
    bool any{false};

    for (int index = 0; index < d.level_size() * d.levels; ++index)
    {
        const auto c = ops.search(index);
        if (!c || !is_open(open, index))
        {
            continue;
        }
        any = true;

        std::shared_ptr<cell> candidates[2];
        int n = 0;
        if (auto north = ops.get_north(c); is_open(ops, open, north))
        {
            candidates[n++] = std::move(north);
        }
        if (auto east = ops.get_east(c); is_open(ops, open, east))
        {
            candidates[n++] = std::move(east);
        }
        if (n > 0)
        {
            link(c, candidates[rng(0, n - 1)], true);
        }
    }
    return any;
}

bool lab::sidewinder(grid_operations &ops, randomizer &rng, const lab::cell_filter &open)
{
    const auto d = dims_of(ops);
    bool any{false};

    for (int lv = 0; lv < d.levels; ++lv)
    {
        for (int row = 0; row < d.rows; ++row)
        {
            std::vector<std::shared_ptr<cell>> run;
            for (int col = 0; col < d.cols; ++col)
            {
                const int index = lv * d.level_size() + row * d.cols + col;
                const auto c = ops.search(index);
                if (!c || !is_open(open, index))
                {
                    continue;
                }
                any = true;
                run.push_back(c);

                const auto east = ops.get_east(c);
                const bool at_east = !is_open(ops, open, east);
                const bool at_north = !is_open(ops, open, ops.get_north(c));

                if (at_east || (!at_north && rng(0, 1) == 0))
                {
                    if (!at_north)
                    {
                        const auto &member = run.at(static_cast<std::size_t>(rng(0, static_cast<int>(run.size()) - 1)));
                        if (auto north = ops.get_north(member); is_open(ops, open, north))
                        {
                            link(member, north, true);
                        }
                    }
                    run.clear();
                }
                else
                {
                    link(c, east, true);
                }
            }
        }
    }
    return any;
}

bool lab::dfs(grid_operations &ops, randomizer &rng, const lab::cell_filter &open)
{
    const auto d = dims_of(ops);

    // Each level is an independent maze
    for (int lv = 0; lv < d.levels; ++lv)
    {
        const auto start = pick_start(ops, open, lv * d.level_size(), d.level_size(), rng);
        if (!start)
        {
            return false;
        }

        std::stack<std::shared_ptr<cell>> stk;
        std::unordered_set<int> visited;
        stk.push(start);
        visited.insert(ops.index_of(start));

        grid_operations::neighbor_buffer neighbor_buf;
        while (!stk.empty())
        {
            const auto current = stk.top();
            const auto count = ops.collect_neighbors(current, neighbor_buf);

            std::vector<std::shared_ptr<cell>> unvisited;
            for (const auto &n : std::span(neighbor_buf).first(count))
            {
                if (is_open(ops, open, n) && !visited.contains(ops.index_of(n)))
                {
                    unvisited.push_back(n);
                }
            }

            if (unvisited.empty())
            {
                stk.pop();
                continue;
            }

            const auto &chosen = unvisited.at(static_cast<std::size_t>(rng(0, static_cast<int>(unvisited.size()) - 1)));
            link(current, chosen, true);
            visited.insert(ops.index_of(chosen));
            stk.push(chosen);
        }
    }
    return true;
}

bool lab::prims(grid_operations &ops, randomizer &rng, const lab::cell_filter &open)
{
    const auto d = dims_of(ops);

    // Each level is an independent maze
    for (int lv = 0; lv < d.levels; ++lv)
    {
        const auto start = pick_start(ops, open, lv * d.level_size(), d.level_size(), rng);
        if (!start)
        {
            return false;
        }

        std::unordered_set<int> in_maze;
        std::unordered_set<int> frontier_ids;
        std::vector<std::shared_ptr<cell>> frontier;

        grid_operations::neighbor_buffer neighbor_buf;
        auto add_frontier_neighbors = [&](const std::shared_ptr<cell> &c)
        {
            const auto count = ops.collect_neighbors(c, neighbor_buf);
            for (const auto &n : std::span(neighbor_buf).first(count))
            {
                if (!is_open(ops, open, n))
                {
                    continue;
                }

                const auto idx = ops.index_of(n);
                if (!in_maze.contains(idx) && frontier_ids.insert(idx).second)
                {
                    frontier.push_back(n);
                }
            }
        };

        in_maze.insert(ops.index_of(start));
        add_frontier_neighbors(start);

        while (!frontier.empty())
        {
            const auto pos = static_cast<std::size_t>(rng(0, static_cast<int>(frontier.size()) - 1));
            const auto current = frontier.at(pos);

            frontier.at(pos) = frontier.back();
            frontier.pop_back();
            frontier_ids.erase(ops.index_of(current));

            // Frontier cells are accepted, and only accepted cells ever enter the maze
            std::vector<std::shared_ptr<cell>> neighbors_in_maze;
            const auto count = ops.collect_neighbors(current, neighbor_buf);
            for (const auto &n : std::span(neighbor_buf).first(count))
            {
                if (n && in_maze.contains(ops.index_of(n)))
                {
                    neighbors_in_maze.push_back(n);
                }
            }

            if (neighbors_in_maze.empty())
            {
                continue;
            }

            const auto chosen = static_cast<std::size_t>(rng(0, static_cast<int>(neighbors_in_maze.size()) - 1));
            link(current, neighbors_in_maze.at(chosen), true);
            in_maze.insert(ops.index_of(current));

            add_frontier_neighbors(current);
        }
    }
    return true;
}
