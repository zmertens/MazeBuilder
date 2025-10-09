#ifndef MAZE_BUILDER_H
#define MAZE_BUILDER_H

/// @file maze_builder.h
/// @brief This file includes all the headers in the maze builder library

#include <MazeBuilder/algo_interface.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/base64_helper.h>
#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create.h>
#include <MazeBuilder/create2.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/distances.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/grid_range.h>
#include <MazeBuilder/hash_funcs.h>
#include <MazeBuilder/io_utils.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/maze_factory.h>
#include <MazeBuilder/maze_interface.h>
#include <MazeBuilder/objectify.h>
#include <MazeBuilder/pixels.h>
#include <MazeBuilder/progress.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/singleton_base.h>
#include <MazeBuilder/stringify.h>
#include <MazeBuilder/string_utils.h>
#include <MazeBuilder/wavefront_object_helper.h>

namespace mazes
{

    /// @brief Version string for the maze builder
    static const std::string VERSION = buildinfo::Version + "-" + buildinfo::CommitSHA;

} // namespace

#endif // MAZE_BUILDER_H
