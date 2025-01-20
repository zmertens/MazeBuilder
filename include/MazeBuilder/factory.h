#ifndef MAZE_FACTORY_H
#define MAZE_FACTORY_H

#include <functional>
#include <memory>
#include <random>

#include <MazeBuilder/enums.h>

namespace mazes {

class grid_interface;

class factory {
public:
	static bool gen(algos maze_type, std::unique_ptr<grid_interface>& _grid, const std::function<int(int, int)>& get_int, const std::mt19937& rng);
};

} // namespace mazes

#endif // MAZE_FACTORY_H
