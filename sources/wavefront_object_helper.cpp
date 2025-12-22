#include <MazeBuilder/wavefront_object_helper.h>

#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/objectify.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/string_utils.h>

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <sstream>

using namespace mazes;

bool wavefront_object_helper::run(grid_interface *g, randomizer &rng) const noexcept
{
    using namespace std;

    auto&& g_ops = g->operations();

    if (g_ops.get_vertices().empty() || g_ops.get_faces().empty())
    {
        if (const mazes::objectify obj_tool{}; !obj_tool.run(g, std::ref(rng)))
        {
            return false;
        }
        if (g_ops.get_vertices().empty() || g_ops.get_faces().empty())
        {
            return false;
        }
    }

    ostringstream result;

    // Write header
    result << "# mazebuilder v" << buildinfo::Version << "-" << buildinfo::CommitSHA << "\n";

    // Write vertices - use direct stream output to avoid string conversions
    for (const auto &vertex : g_ops.get_vertices())
    {
        float x = static_cast<float>(get<0>(vertex));
        float y = static_cast<float>(get<1>(vertex));
        float z = static_cast<float>(get<2>(vertex));
        result << "v " << x << " " << y << " " << z << "\n";
    }

    // Write faces - minimize string operations
    for (const auto &face : g_ops.get_faces())
    {
        result << "f";
        for (const auto index : face)
        {
            result << " " << index;
        }
        result << "\n";
    }

    g_ops.set_file(result.str());

    return !g_ops.get_file().empty();
} // run
