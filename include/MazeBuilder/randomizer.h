#ifndef RANDOMIZER_H
#define RANDOMIZER_H

#include <vector>
#include <memory>

/// @brief Namespace for the maze builder
namespace mazes {

/// @file randomizer.h

/// @class randomizer
/// @brief Provides random-number generating capabilities
/// @details This class provides methods for generating random numbers
class randomizer {
public:

    /// @brief Default constructor
    randomizer();

    /// @brief Destructor
    ~randomizer();

    /// @brief Copy constructor
    /// @param other The randomizer object to copy from
    randomizer(const randomizer& other);

    /// @brief Copy assignment operator
    /// @param other The randomizer object to copy from
    /// @return Reference to the current object
    randomizer& operator=(const randomizer& other);

    /// @brief Move constructor
    /// @param other The randomizer object to move from
    randomizer(randomizer&& other) noexcept;

    /// @brief Move assignment operator
    /// @param other The randomizer object to move from
    /// @return Reference to the current object
    randomizer& operator=(randomizer&& other) noexcept;

    /// @brief Generates a random integer within a specified range.
    /// @param low The lower bound of the integer (inclusive).
    /// @param high The upper bound of the integer (inclusive).
    /// @return A random integer between the specified range [low, high].
    int get_int(int low = 0, int high = 1) noexcept;

    /// @brief Generates a shuffled vector of all integers in the specified range
    /// @param low The lower bound of the integer(s) (inclusive).
    /// @param high The upper bound of the integer(s) (inclusive).
    /// @param count The number of random integers to generate
    /// @return A vector containing all integers in [low, high] in random order
    std::vector<int> get_vector_ints(int low = 0, int high = 1, int count = 1) noexcept;

    /// @brief Seeds the random number generator with the given seed value.
    /// @param seed The seed value to initialize the random number generator.
    void seed(unsigned long long seed = 0) noexcept;

    /// @brief Gets a random integer within a specified range.
    /// @param low The lower bound of the integer (inclusive).
    /// @param high The upper bound of the integer (inclusive).
    /// @return A random integer within the specified range.
    int operator()(int low, int high) noexcept {

        return get_int(low, high);
    }

private:
    class randomizer_impl;
    
    std::unique_ptr<randomizer_impl> m_impl;
};

}
#endif // RANDOMIZER_H
