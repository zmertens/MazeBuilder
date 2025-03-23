#include <MazeBuilder/randomizer.h>

using namespace mazes;

/// @brief 
/// @param seed std::seed_seq{ 1, 2, 3, 4, 5 }
randomizer<std::mt19937>::randomizer(unsigned long long seed) {

}

int randomizer<std::mt19937>::get_int(int low, int high) const noexcept {
    std::uniform_int_distribution<int> dist{ low, high };
    return dist(rng_device);
}
