#include <MazeBuilder/masked_maze_and_create_state.h>
#include <MazeBuilder/algos.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/mask.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/resource_identifiers.h>
#include <MazeBuilder/runtime_stack.h>
#include <filesystem>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

using namespace mazes;

masked_maze_and_create_state::masked_maze_and_create_state(const runtime_app::context& ctx, runtime_stack* rs)
    : link_maze_and_create_state(ctx, rs)
{
}

algo masked_maze_and_create_state::get_algo_id() const noexcept
{
    return algo::BINARY_TREE;
}

std::string_view masked_maze_and_create_state::create(const configurator& config, randomizer& rng) noexcept
{
    const auto mask_file = config.mask_file();
    if (mask_file.empty())
        return "Error: No mask file specified";
    return create_masked_maze(mask_file, config.algo_id(), rng);
}

std::string_view masked_maze_and_create_state::create_masked_maze(const std::string& mask_file,
    algo algorithm, randomizer& rng) noexcept
{
    if (!grid_mapper)
        return "Error: Grid mapper not available";

    try
    {
        if (!std::filesystem::exists(mask_file))
        {
            m_result = "Error: Mask file not found: " + mask_file;
            return m_result;
        }

        auto loaded_mask = mask::from_txt(mask_file);
        const auto rows = loaded_mask.rows();
        const auto cols = loaded_mask.columns();

        auto& grid_ref = grid_mapper->get(current_grid_id);
        auto& grid_ops = grid_ref.operations();

        switch (algorithm)
        {
        case algo::BINARY_TREE:
            for (unsigned int row = 0; row < rows; ++row)
            {
                for (unsigned int col = 0; col < cols; ++col)
                {
                    if (!loaded_mask(row, col))
                        continue;
                    
                    auto c = grid_ops.search(static_cast<int>(row * cols + col));
                    if (!c)
                        continue;

                    std::vector<std::shared_ptr<cell>> cands;
                    
                    auto n = grid_ops.get_north(c);
                    if (n && loaded_mask(n->get_index() / cols, n->get_index() % cols))
                        cands.push_back(n);
                    
                    auto e = grid_ops.get_east(c);
                    if (e && loaded_mask(e->get_index() / cols, e->get_index() % cols))
                        cands.push_back(e);
                    
                    if (!cands.empty())
                        lab::link(c, cands[rng(0, static_cast<int>(cands.size()) - 1)], true);
                }
            }
            break;

        case algo::SIDEWINDER:
            for (unsigned int row = 0; row < rows; ++row)
            {
                std::vector<std::shared_ptr<cell>> run;
                for (unsigned int col = 0; col < cols; ++col)
                {
                    if (!loaded_mask(row, col))
                        continue;
                    
                    auto c = grid_ops.search(static_cast<int>(row * cols + col));
                    if (!c)
                        continue;

                    run.push_back(c);

                    auto e = grid_ops.get_east(c);
                    bool at_east = !e || !loaded_mask(e->get_index() / cols, e->get_index() % cols);
                    
                    auto n = grid_ops.get_north(c);
                    bool at_north = !n || !loaded_mask(n->get_index() / cols, n->get_index() % cols);

                    if (at_east || (!at_north && rng(0, 1) == 0))
                    {
                        if (!run.empty())
                        {
                            auto& member = run[rng(0, static_cast<int>(run.size()) - 1)];
                            auto mn = grid_ops.get_north(member);
                            if (mn && loaded_mask(mn->get_index() / cols, mn->get_index() % cols))
                                lab::link(member, mn, true);
                        }
                        run.clear();
                    }
                    else if (e && loaded_mask(e->get_index() / cols, e->get_index() % cols))
                    {
                        lab::link(c, e, true);
                    }
                }
            }
            break;

        case algo::DFS:
        {
            std::shared_ptr<cell> start = nullptr;
            for (unsigned int r = 0; r < rows && !start; ++r)
            {
                for (unsigned int c = 0; c < cols && !start; ++c)
                {
                    if (loaded_mask(r, c))
                        start = grid_ops.search(static_cast<int>(r * cols + c));
                }
            }
            
            if (!start)
            {
                m_result = "Error: No available cells in mask";
                return m_result;
            }

            std::stack<std::shared_ptr<cell>> stk;
            std::unordered_set<int> visited;
            stk.push(start);
            visited.insert(start->get_index());

            while (!stk.empty())
            {
                auto current = stk.top();
                std::vector<std::shared_ptr<cell>> unvisited;
                
                for (auto& n : grid_ops.get_neighbors(current))
                {
                    if (n && loaded_mask(n->get_index() / cols, n->get_index() % cols) && !visited.contains(n->get_index()))
                        unvisited.push_back(n);
                }

                if (unvisited.empty())
                {
                    stk.pop();
                }
                else
                {
                    auto& chosen = unvisited[rng(0, static_cast<int>(unvisited.size()) - 1)];
                    lab::link(current, chosen, true);
                    visited.insert(chosen->get_index());
                    stk.push(chosen);
                }
            }
            break;
        }

        case algo::PRIMS:
        {
            std::vector<std::shared_ptr<cell>> open_cells;
            open_cells.reserve(rows * cols);
            for (unsigned int row = 0; row < rows; ++row)
            {
                for (unsigned int col = 0; col < cols; ++col)
                {
                    if (!loaded_mask(row, col))
                    {
                        continue;
                    }

                    if (auto c = grid_ops.search(static_cast<int>(row * cols + col)))
                    {
                        open_cells.push_back(c);
                    }
                }
            }

            if (open_cells.empty())
            {
                m_result = "Error: No available cells in mask";
                return m_result;
            }

            std::unordered_set<int> in_maze;
            std::unordered_set<int> frontier_ids;
            std::vector<std::shared_ptr<cell>> frontier;

            auto add_frontier_neighbors = [&](const std::shared_ptr<cell>& c)
                {
                    for (const auto& n : grid_ops.get_neighbors(c))
                    {
                        if (!n)
                        {
                            continue;
                        }

                        const auto idx = n->get_index();
                        const auto nr = static_cast<unsigned int>(idx / cols);
                        const auto nc = static_cast<unsigned int>(idx % cols);
                        if (!loaded_mask(nr, nc))
                        {
                            continue;
                        }

                        if (!in_maze.contains(idx) && frontier_ids.insert(idx).second)
                        {
                            frontier.push_back(n);
                        }
                    }
                };

            auto start = open_cells.at(static_cast<std::size_t>(rng(0, static_cast<int>(open_cells.size()) - 1)));
            in_maze.insert(start->get_index());
            add_frontier_neighbors(start);

            while (!frontier.empty())
            {
                const auto frontier_pos = static_cast<std::size_t>(rng(0, static_cast<int>(frontier.size()) - 1));
                auto current = frontier.at(frontier_pos);

                frontier.at(frontier_pos) = frontier.back();
                frontier.pop_back();
                frontier_ids.erase(current->get_index());

                std::vector<std::shared_ptr<cell>> neighbors_in_maze;
                for (const auto& n : grid_ops.get_neighbors(current))
                {
                    if (!n)
                    {
                        continue;
                    }

                    const auto idx = n->get_index();
                    const auto nr = static_cast<unsigned int>(idx / cols);
                    const auto nc = static_cast<unsigned int>(idx % cols);
                    if (loaded_mask(nr, nc) && in_maze.contains(idx))
                    {
                        neighbors_in_maze.push_back(n);
                    }
                }

                if (neighbors_in_maze.empty())
                {
                    continue;
                }

                auto chosen = neighbors_in_maze.at(static_cast<std::size_t>(rng(0, static_cast<int>(neighbors_in_maze.size()) - 1)));
                lab::link(current, chosen, true);
                in_maze.insert(current->get_index());

                add_frontier_neighbors(current);
            }
            break;
        }
        default:
            m_result = "Error: Unsupported algorithm for masked maze";
            return m_result;
        }

        if (m_use_distances)
        {
            if (auto* dg = dynamic_cast<distance_grid*>(&grid_ref))
                dg->calculate_distances(m_distances_start, m_distances_end);
        }

        m_result = "Masked maze generated from " + mask_file;
        return m_result;
    }
    catch (const std::exception& e)
    {
        m_result = "Error creating masked maze: " + std::string(e.what());
        return m_result;
    }
    catch (...)
    {
        return "Unknown error creating masked maze";
    }
}

