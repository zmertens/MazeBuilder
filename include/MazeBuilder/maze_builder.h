#ifndef MAZE_BUILDER_H
#define MAZE_BUILDER_H

/// @file maze_builder.h
/// @brief This file includes all the headers in the maze builder library

#include <MazeBuilder/args.h>
#include <MazeBuilder/binary_tree.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/distances.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/factory.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/json_helper.h>
#include <MazeBuilder/lab.h>
#include <MazeBuilder/maze.h>
#include <MazeBuilder/progress.h>
#include <MazeBuilder/sidewinder.h>
#include <MazeBuilder/stringz.h>
#include <MazeBuilder/wavefront_object_helper.h>
#include <MazeBuilder/writer.h>

namespace mazes {

/// @brief Version string for the maze builder
static const std::string VERSION = build_info::Version + "-" + build_info::CommitSHA;

} // namespace

#endif // MAZE_BUILDER_H

