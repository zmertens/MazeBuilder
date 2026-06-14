#include <MazeBuilder/wavefront_object_create_state.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/runtime_stack.h>

#include <fmt/format.h>

using namespace mazes;

wavefront_object_create_state::wavefront_object_create_state(const runtime_app::context &ctx, runtime_stack *stack)
: state(ctx, stack), grid_mapper{ctx.get_grid_manager()}, processed_text_mapper{ctx.get_text_manager()}
{

}

std::string_view wavefront_object_create_state::create(algo a, unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept
{
    if (a != algo::WAVEFRONT_OBJECT)
    {
        return std::string_view{"Unsupported algorithm"};
    }

    return create_wavefront_object(rows, cols, levels, std::ref(rng));   
}

void wavefront_object_create_state::draw() const noexcept
{
    // Implementation of the draw function
}

bool wavefront_object_create_state::update([[maybe_unused]] const std::optional<args> &args, [[maybe_unused]] double delta_time) noexcept
{
    // Implementation of the update function
    return true;
}

std::string_view wavefront_object_create_state::create_wavefront_object(unsigned int rows, unsigned int cols, unsigned int levels, randomizer &rng) noexcept
{
    auto sum = rows + cols + levels;
    [[maybe_unused]] auto sum1 = rng(0, sum);
    // if (!g)
    //    auto&& g_ops = g->operations();

    //     if (g_ops.get_vertices().empty() || g_ops.get_faces().empty())
    //     {
    //         if (const mazes::objectify obj_tool{}; !obj_tool.run(g, std::ref(rng)))
    //         {
    //             return false;
    //         }
    //         if (g_ops.get_vertices().empty() || g_ops.get_faces().empty())
    //         {
    //             return false;
    //         }
    //     }

    //     ostringstream result;

    //     // Write header
    //     result << "# mazebuilder v" << buildinfo::Version << "-" << buildinfo::CommitSHA << "\n";

    //     // Write vertices - use direct stream output to avoid string conversions
    //     for (const auto &vertex : g_ops.get_vertices())
    //     {
    //         float x = static_cast<float>(get<0>(vertex));
    //         float y = static_cast<float>(get<1>(vertex));
    //         float z = static_cast<float>(get<2>(vertex));
    //         result << "v " << x << " " << y << " " << z << "\n";
    //     }

    //     // Write faces - minimize string operations
    //     for (const auto &face : g_ops.get_faces())
    //     {
    //         result << "f";
    //         for (const auto index : face)
    //         {
    //             result << " " << index;
    //         }
    //         result << "\n";
    //     }

    //     g_ops.set_file(result.str());

    //     return !g_ops.get_file().empty();
   

    return std::string_view{"Wavefront Object Maze generation not implemented yet"};
}
