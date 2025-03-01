#ifndef SIDEWINDER_HPP
#define SIDEWINDER_HPP

#include <MazeBuilder/algo_interface.h>

#include <functional>
#include <memory>
#include <random>

namespace mazes {

class grid_interface;

class sidewinder : public algo_interface {
    public:
    /// @brief Implement the sidewinder maze generation algorithm
    /// @param g 
    /// @param get_int 
    /// @param rng 
    /// @return 
    bool run(std::unique_ptr<grid_interface> const& g, const std::function<int(int, int)>& get_int, const std::mt19937& rng) const noexcept override;
};

}

#endif // SIDEWINDER_HPP
