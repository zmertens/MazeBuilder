#ifndef MAZE_FACTORY_H
#define MAZE_FACTORY_H

#include "maze_types_enum.h"
#include <functional>
#include <memory>
#include <random>

namespace mazes {

class grid;

class maze_factory {
public:
	static bool gen_maze(maze_types maze_type, std::unique_ptr<grid>& _grid, const std::function<int(int, int)>& get_int, const std::mt19937& rng);
};

} // namespace mazes

#endif // MAZE_FACTORY_H
