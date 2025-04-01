#include <MazeBuilder/randomizer.h>

#include <random>
#include <functional>
#include <array>

using namespace mazes;

class randomizer::randomizer_impl {
private:
    std::mt19937 rng_device;

public:
    randomizer_impl() : rng_device{ std::random_device{}() } {}

    template <typename Number = int>
    Number get_int(Number low, Number high) const noexcept {
        std::uniform_int_distribution<Number> dist{ low, high };
        return dist(rng_device);
    }

    void seed() {
        std::random_device rd;
        std::array<int, std::mt19937::state_size> seed_data;
        std::generate(seed_data.begin(), seed_data.end(), std::ref(rd));
        std::seed_seq seq(seed_data.begin(), seed_data.end());
        rng_device.seed(seq);
    }

    void seed(unsigned long long seed) noexcept {
        rng_device.seed(seed);
    }
};

randomizer::randomizer() : m_impl{ std::make_unique<randomizer_impl>() } {}

//void randomizer::seed() noexcept {
//    this->m_impl->seed();
//}

/// @brief 
/// @param seed
//void randomizer::seed(unsigned long long seed) noexcept {
//    this->m_impl->seed(seed);
//}

/// @brief Generates a random integer within a specified range.
/// @param low The lower bound of the range (inclusive).
/// @param high The upper bound of the range (inclusive).
/// @return A random integer between the specified low and high bounds (inclusive).
//int randomizer::get_int(int low, int high) const noexcept {
//    return this->m_impl->get_int(low, high);
//}
