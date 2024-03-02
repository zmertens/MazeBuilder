#ifndef SIDEWINDER_HPP
#define SIDEWINDER_HPP

#include "maze_algo_interface.h"

class grid;

class sidewinder : public maze_algo_interface {
public:

    bool run(grid& g, std::function<int(int, int)> const& get_int) const noexcept override;
private:

};

#endif // SIDEWINDER_HPP