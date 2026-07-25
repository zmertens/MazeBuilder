#include <MazeBuilder/randomizer.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <random>
#include <ranges>

using namespace mazes;

class randomizer::randomizer_impl
{
    std::mt19937 rng_device;

public:
    randomizer_impl()
#if defined(__EMSCRIPTEN__)
        : rng_device{5489u}
#else
        : rng_device{std::random_device{}()}
#endif
    {
    }

    template <typename Number = std::int32_t>
    Number get_range(Number low, Number high) noexcept
    {
        if constexpr (std::is_integral_v<Number>)
        {
            std::uniform_int_distribution<Number> dist{low, high};
            return dist(rng_device);
        }
        else
        {
            std::uniform_real_distribution<Number> dist{low, high};
            return dist(rng_device);
        }
    }

    // Generate a vector of integers within the specified range
    [[nodiscard]] std::vector<int> get_vector_ints(int low, int high, const int count) noexcept
    {
        // Handle invalid ranges
        if (low > high || count <= 0)
        {
            return {};
        }

        std::vector<int> numbers;

        numbers.reserve(count);

        for (int i = 0; i < count; ++i)
        {
            numbers.emplace_back(get_range<int>(low, high));
        }

        // Shuffle the vector using the random number generator
        std::ranges::shuffle(numbers, rng_device);

        return numbers;
    }

    void seed() noexcept
    {
#if defined(__EMSCRIPTEN__)
        const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        rng_device.seed(static_cast<std::mt19937::result_type>(now));
#else
        std::random_device rd;
        std::array<int, std::mt19937::state_size> seed_data{};
        std::ranges::generate(seed_data, std::ref(rd));
        std::seed_seq seq(seed_data.begin(), seed_data.end());

        rng_device.seed(seq);
#endif
    }

    void seed(const unsigned long long seed) noexcept
    {
        rng_device.seed(static_cast<std::mt19937::result_type>(seed));
    }
};

// Default constructor
randomizer::randomizer() : m_impl{std::make_unique<randomizer_impl>()}
{
}

// Copy constructor
randomizer::randomizer(const randomizer& other)
    : m_impl{std::make_unique<randomizer_impl>(*other.m_impl)}
{
}

// Copy assignment operator
randomizer& randomizer::operator=(const randomizer& other)
{
    if (this == &other)
    {
        // Handle self-assignment
        return *this;
    }

    m_impl = std::make_unique<randomizer_impl>(*other.m_impl);

    return *this;
}

// Move constructor
randomizer::randomizer(randomizer&& other) noexcept
    : m_impl{std::move(other.m_impl)}
{
}

// Move assignment operator
randomizer& randomizer::operator=(randomizer&& other) noexcept
{
    if (this == &other)
    {
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
void randomizer::seed(const unsigned long long seed) const noexcept
{
    if (seed == 0)
    {
        this->m_impl->seed();
        return;
    }

    this->m_impl->seed(seed);
}

/// @brief Generates a random integer within a specified range.
/// @param low defaults to 0
/// @param high defaults to 1
/// @return A random integer between the specified low and high bounds (inclusive).
int randomizer::get_int(const int low, const int high) const noexcept
{
    return this->m_impl->get_range<int>(low, high);
}

/// @brief Generates a random float within a specified range.
/// @param low defaults to 0.0f
/// @param high defaults to 1.0f
/// @return A random float between the specified low and high bounds (inclusive).
float randomizer::get_float(const float low, const float high) const noexcept
{
    return this->m_impl->get_range<float>(low, high);
}

/// @brief Generates a vector of ints
/// @param low defaults to 0
/// @param high defaults to 1
/// @param count defaults to 1
/// @return A vector of random integers within the specified range.
std::vector<int> randomizer::get_vector_ints(const int low, const int high, const int count) const noexcept
{
    return this->m_impl->get_vector_ints(low, high, count);
}
