#ifndef PIXELS_H
#define PIXELS_H

#include <MazeBuilder/algo_interface.h>

namespace mazes {

class pixels : public algo_interface {
public:
    virtual bool run(std::unique_ptr<grid_interface> const& g, randomizer& rng) const noexcept override;
};
}

#endif // PIXELS_H
