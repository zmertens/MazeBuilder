#ifndef MAZE_ALGO_INTERFACE_HPP
#define MAZE_ALGO_INTERFACE_HPP

#include <memory>
#include <functional>
#include <random>

namespace mazes {

class grid_interface;

class algos_interface {
public:
    virtual bool run(const std::unique_ptr<grid_interface>& gcvt, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept = 0;
};
}
#endif // MAZE_ALGO_INTERFACE_HPP
