#ifndef RANDOMIZER_H
#define RANDOMIZER_H

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
    ~randomizer() = default;

    /// @brief Generates a random integer within a specified range.
    /// @param low The lower bound of the range (inclusive).
    /// @param high The upper bound of the range (inclusive).
    /// @return A random integer between the specified range [low, high].
    //int get_int(int low, int high) const noexcept;

    /// @brief Seeds the random number generator.
    //void seed() noexcept;

    /// @brief Seeds the random number generator with the given seed value.
    /// @param seed The seed value to initialize the random number generator.
    //void seed(unsigned long long seed) noexcept;

    /// @brief Gets a random integer within a specified range.
    /// @param low 
    /// @param high 
    /// @return 
    //int operator()(int low, int high) const noexcept {
    //    return get_int(low, high);
    //}

private:
    class randomizer_impl;
    std::unique_ptr<randomizer_impl> m_impl;
};

}
#endif // RANDOMIZER_H
