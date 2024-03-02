#ifndef MAZE_ALGO_INTERFACE_HPP
#define MAZE_ALGO_INTERFACE_HPP

namespace mazes {

#include <functional>

class grid;

class maze_algo_interface {
public:
    virtual bool run(grid& g, std::function<int(int, int)> const& get_int) const noexcept = 0;
};
}
#endif // MAZE_ALGO_INTERFACE_HPP