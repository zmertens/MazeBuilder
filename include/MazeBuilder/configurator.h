#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

#include <string>

#include <MazeBuilder/enums.h>

namespace mazes {

/// @file configurator.h
/// @class configurator
/// @brief Configuration class for grids
class configurator {
public:
    
    /// @brief Set the number of rows
    /// @param rows The number of rows
    /// @return A reference to this configurator
    configurator& rows(unsigned int rows) noexcept {
        m_rows = rows;
        return *this;
    }

    /// @brief Set the number of columns
    /// @param columns The number of columns
    /// @return A reference to this configurator
    configurator& columns(unsigned int columns) noexcept {
        m_columns = columns;
        return *this;
    }

    /// @brief Set the number of levels
    /// @param levels The number of levels
    /// @return A reference to this configurator
    configurator& levels(unsigned int levels) noexcept {
        m_levels = levels;
        return *this;
    }

    /// @brief Set the maze generation algorithm
    /// @param algorithm The algorithm to use
    /// @return A reference to this configurator
    configurator& algo_id(algo algorithm) noexcept {
        m_algo = algorithm;
        return *this;
    }

    /// @brief Set the block ID
    /// @param block_id The block ID
    /// @return A reference to this configurator
    configurator& block_id(int block_id) noexcept {
        m_block_id = block_id;
        return *this;
    }

    /// @brief Set the random seed
    /// @param seed The random seed
    /// @return A reference to this configurator
    configurator& seed(unsigned int seed) noexcept {
        m_seed = seed;
        return *this;
    }

    /// @brief Set the distance calculation flag
    /// @param distances The distance calculation flag
    /// @return A reference to this configurator
    configurator& distances(bool distances) noexcept {
        m_distances = distances;
        return *this;
    }

    /// @brief Set the output ID
    /// @param output The output ID
    /// @return A reference to this configurator
    configurator& output_id(output output) noexcept {
        m_output = output;
        return *this;
    }

    /// @brief Get the number of rows
    /// @return The number of rows
    unsigned int rows() const noexcept { return m_rows; }

    /// @brief Get the number of columns
    /// @return The number of columns
    unsigned int columns() const noexcept { return m_columns; }

    /// @brief Get the number of levels
    /// @return The number of levels
    unsigned int levels() const noexcept { return m_levels; }

    /// @brief Get the maze generation algorithm
    /// @return The algorithm used for maze generation
    algo algo_id() const noexcept { return m_algo; }

    /// @brief Get the block ID
    /// @return The block ID
    int block_id() const noexcept { return m_block_id; }

    /// @brief Get the random seed
    /// @return The random seed
    unsigned int seed() const noexcept { return m_seed; }

    /// @brief Check if distances are calculated
    /// @return True if distances are calculated, false otherwise
    bool distances() const noexcept { return m_distances; }

    /// @brief Get the output ID
    /// @return The output ID
    output output_id() const noexcept { return m_output; }

private:
    unsigned int m_rows;

    unsigned int m_columns;

    unsigned int m_levels;

    int m_block_id;

    algo m_algo;

    unsigned int m_seed;

    bool m_distances;

    output m_output;
};

} // namespace

#endif // CONFIGURATOR_H
