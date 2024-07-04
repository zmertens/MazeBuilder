#ifndef MAZE_FACTORY_H
#define MAZE_FACTORY_H

#include "maze_types_enum.h"
#include <functional>
#include <memory>

namespace mazes {

class grid;

class maze_factory {
public:
	static bool gen_maze(maze_types maze_type, std::unique_ptr<grid>& _grid, std::function<int(int, int)> get_int);
};

} // namespace mazes

#endif // MAZE_FACTORY_H
