#ifndef MAZE_ALGO_INTERFACE_HPP
#define MAZE_ALGO_INTERFACE_HPP

#include <memory>
#include <functional>
#include <future>

namespace mazes {

class grid;

class maze_algo_interface {
public:
    virtual bool run(const std::unique_ptr<mazes::grid>& _grid, const std::function<int(int, int)>& get_int, bool interactive = false) const noexcept = 0;
};
}
#endif // MAZE_ALGO_INTERFACE_HPP
