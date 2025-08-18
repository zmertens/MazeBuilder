#include <MazeBuilder/randomizer.h>

#include <random>
#include <functional>
#include <algorithm>
#include <array>

using namespace mazes;

class randomizer::randomizer_impl {
private:
    std::mt19937 rng_device;

public:
    randomizer_impl() : rng_device{ std::random_device{}() } {}

    template <typename Number = int>
    Number get_int_incl(Number low, Number high) noexcept {
        std::uniform_int_distribution<Number> dist{ low, high };
        return dist(rng_device);
    }

    /// @brief Generate a random range of integers
    /// @param low
    /// @param high
    /// @return 
    std::vector<int> get_num_ints_incl(int low, int high) noexcept {
        // Handle invalid ranges
        if (low > high) {
            return {};
        }

        // Calculate the range size
        int rangeSize = high - low + 1;
        
        // Generate a vector of integers within the specified range
        std::vector<int> numbers;
        numbers.reserve(rangeSize);
        for (int i = low; i <= high; ++i) {
            numbers.emplace_back(i);
        }

        // Shuffle the vector using the random number generator
        std::shuffle(numbers.begin(), numbers.end(), rng_device);

        return numbers;
    }

    void seed(unsigned long long seed = 0) noexcept {
        if (seed != 0) {

            rng_device.seed(static_cast<std::mt19937::result_type>(seed));
            return;
        }

        std::random_device rd;
        std::array<int, std::mt19937::state_size> seed_data;
        std::generate(seed_data.begin(), seed_data.end(), std::ref(rd));
        std::seed_seq seq(seed_data.begin(), seed_data.end());
        rng_device.seed(seq);
    }
};

// Default constructor
randomizer::randomizer() : m_impl{ std::make_unique<randomizer_impl>() } {}

// Copy constructor
randomizer::randomizer(const randomizer& other)
    : m_impl{ std::make_unique<randomizer_impl>(*other.m_impl) } {
}

// Copy assignment operator
randomizer& randomizer::operator=(const randomizer& other) {
    if (this == &other) {
        return *this; // Handle self-assignment
    }
    m_impl = std::make_unique<randomizer_impl>(*other.m_impl);
    return *this;
}

// Move constructor
randomizer::randomizer(randomizer&& other) noexcept
    : m_impl{ std::move(other.m_impl) } {
}

// Move assignment operator
randomizer& randomizer::operator=(randomizer&& other) noexcept {
    if (this == &other) {
        return *this; // Handle self-assignment
    }
    m_impl = std::move(other.m_impl);
    return *this;
}

// Destructor
randomizer::~randomizer() = default;

/// @brief 
/// @param seed 0
void randomizer::seed(unsigned long long seed) noexcept {
    if (seed == 0) {
        this->m_impl->seed();
        return;
    } else {
        // Check if the seed is within the valid range
        if (seed < std::numeric_limits<unsigned long long>::min() || seed > std::numeric_limits<unsigned long long>::max()) {
            this->m_impl->seed(seed);
        }
    }
    this->m_impl->seed(seed);
}

/// @brief Generates a random integer within a specified range.
/// @param low The lower bound of the range (inclusive).
/// @param high The upper bound of the range (inclusive).
/// @return A random integer between the specified low and high bounds (inclusive).
int randomizer::get_int_incl(int low, int high) noexcept {

    return this->m_impl->get_int_incl(low, high);
}

/// @brief 
/// @param num 
/// @param low 0
/// @param high 1
/// @return 
std::vector<int> randomizer::get_num_ints_incl(int low, int high) noexcept {

    return this->m_impl->get_num_ints_incl(low, high);
}
