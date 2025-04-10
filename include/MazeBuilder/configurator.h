#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

#include <string>

#include <MazeBuilder/enums.h>

namespace mazes {

/// @file configurator.h

/// @class configurator
/// @brief Configuration class for the maze builder
class configurator {
public:
    
    configurator& rows(unsigned int r) noexcept {
        rows_ = r;
        return *this;
    }

    configurator& columns(unsigned int c) noexcept {
        columns_ = c;
        return *this;
    }

    configurator& levels(unsigned int l) noexcept {
        levels_ = l;
        return *this;
    }

    configurator& _algo(algo _a) noexcept {
        algo_ = _a;
        return *this;
    }

    configurator& block_id(int val) noexcept {
        block_id_ = val;
        return *this;
    }

    configurator& seed(unsigned int s) noexcept {
        seed_ = s;
        return *this;
    }

    configurator& distances(bool d) noexcept {
        distances_ = d;
        return *this;
    }

    configurator& _output(output o) noexcept {
        output_ = o;
        return *this;
    }

    // Getters for the configuration options
    unsigned int rows() const noexcept { return rows_; }
    unsigned int columns() const noexcept { return columns_; }
    unsigned int levels() const noexcept { return levels_; }
    algo _algo() const noexcept { return algo_; }
    int block_id() const noexcept { return block_id_; }
    unsigned int seed() const noexcept { return seed_; }
    bool distances() const noexcept { return distances_; }
    output _output() const noexcept { return output_; }

private:
    unsigned int rows_ = 1;
    unsigned int columns_ = 1;
    unsigned int levels_ = 1;
    int block_id_ = -1;
    algo algo_ = algo::DFS;
    unsigned int seed_ = 2;
    bool distances_ = false;
    output output_ = output::STDOUT;
};

} // namespace

#endif // CONFIGURATOR_H
