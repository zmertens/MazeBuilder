#ifndef STRINGIFY_H
#define STRINGIFY_H

#include <MazeBuilder/algo_interface.h>

namespace mazes {

class stringify : public algo_interface {
public:
    virtual bool run(std::unique_ptr<grid_interface> const& g, randomizer& rng) const noexcept override;
};
}

#endif // STRINGIFY_H
