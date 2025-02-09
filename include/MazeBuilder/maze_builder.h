#ifndef MAZE_BUILDER_H
#define MAZE_BUILDER_H

////////////////////////////////////////////////////////////
/// \defgroup Primary class for handling project includes
///
///
/// 
///
////////////////////////////////////////////////////////////
#include <MazeBuilder/builder.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/computations.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/factory.h>
#include <MazeBuilder/maze.h>
#include <MazeBuilder/progress.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/writer.h>
#include <MazeBuilder/tools.h>

namespace mazes {

static const std::string VERSION = build_info::Version + "-" + build_info::CommitSHA;

} // namespace

#endif // MAZE_BUILDER_H

