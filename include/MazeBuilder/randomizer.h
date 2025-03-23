#ifndef RANDOMIZER_H
#define RANDOMIZER_H

#include <functional>
#include <random>

/// @brief Namespace for the maze builder
namespace mazes {

/// @file randomizer.h

/// @class randomizer
/// @brief Provides random-number generating capabilities
/// @details This interface provides methods for generating random numbers
template <typename RNG = std::mt19937>
class randomizer {
    RNG rng_device;
public:
    explicit randomizer(unsigned long long seed = std::seed_seq{1, 2, 3, 4, 5});
    int get_int(int low, int high) const noexcept;
};
}
#endif // RANDOMIZER_H
