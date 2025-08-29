#include <MazeBuilder/randomizer.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <random>

using namespace mazes;

class randomizer::randomizer_impl {
private:
    std::mt19937 rng_device;

public:
    randomizer_impl() : rng_device{ std::random_device{}() } {}

    template <typename Number = std::int32_t>
    Number get_range(Number low, Number high) noexcept {

        std::uniform_int_distribution<Number> dist{ low, high };

        return dist(rng_device);
    }

    /// @brief Generates a random integer within a specified range.
    /// @param low defaults to 0
    /// @param high defaults to 1
    /// @param count defaults to 1
    /// @return A random integer within the specified range.
    int get_int(int low, int high) noexcept {

        return get_range(low, high);
    }

    // Generate a vector of integers within the specified range
    std::vector<int> get_vector_ints(int low, int high, int count) noexcept {
        // Handle invalid ranges
        if (low > high || count <= 0) {

            return {};
        }
        
        std::vector<int> numbers;

        numbers.reserve(count);

        for (int i = 0; i < count; ++i) {

            numbers.emplace_back(get_range(low, high));
        }

        // Shuffle the vector using the random number generator
        std::shuffle(numbers.begin(), numbers.end(), rng_device);

        return numbers;
    }

    void seed() noexcept {

        std::random_device rd;
        std::array<int, std::mt19937::state_size> seed_data;
        std::generate(seed_data.begin(), seed_data.end(), std::ref(rd));
        std::seed_seq seq(seed_data.begin(), seed_data.end());

        rng_device.seed(seq);
    }

    void seed(unsigned long long seed) noexcept {

        rng_device.seed(static_cast<std::mt19937::result_type>(seed));
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
        // Handle self-assignment
        return *this;
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
        // Handle self-assignment
        return *this;
    }

    m_impl = std::move(other.m_impl);

    return *this;
}

// Destructor
randomizer::~randomizer() = default;

/// @brief Seeds the random number generator.
/// @param seed 0
void randomizer::seed(unsigned long long seed) noexcept {

    // Check if the seed is within the valid range
    if (seed < std::numeric_limits<unsigned long long>::min() || seed > std::numeric_limits<unsigned long long>::max()) {
        
        this->m_impl->seed(seed);

        return;
    }
    
    this->m_impl->seed();
}

/// @brief Generates a random integer within a specified range.
/// @param low defaults to 0
/// @param high defaults to 1
/// @return A random integer between the specified low and high bounds (inclusive).
int randomizer::get_int(int low, int high) noexcept {

    return this->m_impl->get_int(low, high);
}

/// @brief Generates a vector of ints
/// @param low defaults to 0
/// @param high defaults to 1
/// @param count defaults to 1
/// @return A vector of random integers within the specified range.
std::vector<int> randomizer::get_vector_ints(int low, int high, int count) noexcept {

    return this->m_impl->get_vector_ints(low, high, count);
}
