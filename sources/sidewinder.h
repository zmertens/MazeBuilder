#ifndef SIDEWINDER_HPP
#define SIDEWINDER_HPP

#include <functional>
#include <memory>
#include <vector>
#include <random>

#include "maze_algo_interface.h"

namespace mazes {

class grid;
class cell;

class sidewinder : public maze_algo_interface {
public:
    bool run(std::unique_ptr<grid> const& _grid, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept override;
// hide recursive implementation for now
//private:
//    bool run_on_cell(std::shared_ptr<cell> const& _cell, std::function<int(int, int)> const& get_int) const noexcept;
};

}

#endif // SIDEWINDER_HPP
