#ifndef MAZE_ALGO_INTERFACE_HPP
#define MAZE_ALGO_INTERFACE_HPP

#include <memory>
#include <functional>
#include <future>

#include "grid.h"

namespace mazes {

class maze_algo_interface {
public:
    virtual bool run(grid_ptr& _grid, std::function<int(int, int)> const& get_int, bool interactive = false) noexcept = 0;
};
}
#endif // MAZE_ALGO_INTERFACE_HPP
