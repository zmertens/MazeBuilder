#ifndef MAZE_ALGO_INTERFACE_HPP
#define MAZE_ALGO_INTERFACE_HPP

#include <memory>
#include <functional>
#include <future>

namespace mazes {

class grid;

class maze_algo_interface {
public:
    virtual bool run(std::unique_ptr<mazes::grid> const& _grid, std::function<int(int, int)> const& get_int, bool interactive = false) const noexcept = 0;
};
}
#endif // MAZE_ALGO_INTERFACE_HPP
