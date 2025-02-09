#ifndef SIDEWINDER_HPP
#define SIDEWINDER_HPP

#include <MazeBuilder/algos_interface.h>

#include <functional>
#include <memory>
#include <vector>
#include <random>

namespace mazes {

class grid_interface;
class cell;

class sidewinder : public algos_interface {


    public:

    bool run(std::unique_ptr<grid_interface> const& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept override;
};

}

#endif // SIDEWINDER_HPP
