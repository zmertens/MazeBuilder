#ifndef SIDEWINDER_HPP
#define SIDEWINDER_HPP

#include <functional>
#include <memory>
#include <vector>
#include <random>

#include "maze_algo_interface.h"

namespace mazes {

class grid_interface;
class cell;

class sidewinder : public maze_algo_interface {
public:
    bool run(std::unique_ptr<grid_interface> const& _grid, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept override;
};

}

#endif // SIDEWINDER_HPP
