#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

#include <limits>
#include <string>

#include <MazeBuilder/enums.h>

namespace mazes {

/// @file configurator.h
/// @class configurator
/// @brief Configuration class for arguments
/// @details This class stores maze generation parameters with safe default values
/// @warning Always use the constructor or setter methods to ensure proper initialization
class configurator {
    
public:

    static constexpr auto DEFAULT_ROWS = 10u;

    static constexpr auto DEFAULT_COLUMNS = 10u;

    static constexpr auto DEFAULT_LEVELS = 1u;

    static constexpr auto DEFAULT_BLOCK_ID = 0;

    static constexpr auto DEFAULT_ALGO_ID = algo::BINARY_TREE;

    static constexpr auto DEFAULT_SEED = 0u;

    static constexpr auto DEFAULT_OUTPUT_ID = output::PLAIN_TEXT;

    static constexpr auto DEFAULT_DISTANCES = false;

    static constexpr auto MAX_ROWS = 10000u;

    static constexpr auto MAX_COLUMNS = 10000u;

    static constexpr auto MAX_LEVELS = 1000u;

    /// @brief Default constructor - initializes all values to safe defaults
    /// @details Provides reasonable default values to prevent uninitialized memory access
    /// @note Default values: rows=10, columns=10, levels=1, algo=BINARY_TREE, etc.
    configurator() noexcept {

        reset_to_defaults();
    }

    /// @brief Copy constructor
    /// @param other The configurator object to copy from
    configurator(const configurator& other) noexcept = default;

    /// @brief Move constructor  
    /// @param other The configurator object to move from
    configurator(configurator&& other) noexcept = default;

    /// @brief Copy assignment operator
    /// @param other The configurator object to copy from
    /// @return Reference to this configurator
    configurator& operator=(const configurator& other) noexcept = default;

    /// @brief Move assignment operator
    /// @param other The configurator object to move from  
    /// @return Reference to this configurator
    configurator& operator=(configurator&& other) noexcept = default;

    /// @brief Destructor
    ~configurator() noexcept = default;
    
    /// @brief Set the number of rows
    /// @param rows The number of rows (must be > 0, will be clamped to reasonable limits)
    /// @return A reference to this configurator
    /// @warning Values > 10000 will be clamped to prevent memory issues
    configurator& rows(unsigned int rows) noexcept {
        // Clamp to reasonable limits to prevent infinite loops and memory issues
        m_rows = (rows == 0) ? 1 : (rows > MAX_ROWS) ? MAX_ROWS : rows;
        return *this;
    }

    /// @brief Set the number of columns
    /// @param columns The number of columns (must be > 0, will be clamped to reasonable limits)
    /// @return A reference to this configurator
    /// @warning Values > 10000 will be clamped to prevent memory issues
    configurator& columns(unsigned int columns) noexcept {
        // Clamp to reasonable limits to prevent infinite loops and memory issues
        m_columns = (columns == 0) ? 1 : (columns > MAX_COLUMNS) ? MAX_COLUMNS : columns;
        return *this;
    }

    /// @brief Set the number of levels
    /// @param levels The number of levels (must be > 0, will be clamped to reasonable limits)
    /// @return A reference to this configurator
    /// @warning Values > 1000 will be clamped to prevent memory issues
    /// @note Most mazes are 2D (levels=1), 3D mazes should use moderate level counts
    configurator& levels(unsigned int levels) noexcept {
        // Clamp to reasonable limits to prevent infinite loops and memory issues
        // Levels are more memory-intensive than rows/columns, so lower limit
        m_levels = (levels == 0) ? 1 : (levels > MAX_LEVELS) ? MAX_LEVELS : levels;
        return *this;
    }

    /// @brief Set the maze generation algorithm
    /// @param algorithm The algorithm to use
    /// @return A reference to this configurator
    configurator& algo_id(algo algorithm) noexcept {
        m_algo_id = algorithm;
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
    /// @param seed The random seed (0 = use random seed)
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
        m_output_id = output;
        return *this;
    }

    /// @brief Set the help message
    /// @param help The help message
    /// @return A reference to this configurator
    configurator& help(std::string help) noexcept {
        m_help = std::move(help);
        return *this;
    }

    /// @brief Set the version string
    /// @param version The version string
    /// @return A reference to this configurator
    configurator& version(std::string version) noexcept {
        m_version = std::move(version);
        return *this;
    }

    /// @brief Get the number of rows
    /// @return The number of rows (guaranteed to be > 0)
    unsigned int rows() const noexcept { return m_rows; }

    /// @brief Get the number of columns  
    /// @return The number of columns (guaranteed to be > 0)
    unsigned int columns() const noexcept { return m_columns; }

    /// @brief Get the number of levels
    /// @return The number of levels (guaranteed to be > 0)
    unsigned int levels() const noexcept { return m_levels; }

    /// @brief Get the maze generation algorithm
    /// @return The algorithm used for maze generation
    algo algo_id() const noexcept { return m_algo_id; }

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
    output output_id() const noexcept { return m_output_id; }

    /// @brief Get the help message
    /// @return The help message
    const std::string& help() const noexcept { return m_help; }

    /// @brief Get the version string
    /// @return The version string
    const std::string& version() const noexcept { return m_version; }

    /// @brief Validate all configuration values are within safe limits
    /// @return True if all values are valid, false if any are problematic
    /// @details Checks for potential infinite loop conditions and memory issues
    bool is_valid() const noexcept {

        // Check for zero dimensions (would cause infinite loops or divisions by zero)
        if (m_rows == 0 || m_columns == 0 || m_levels == 0) {

            return false;
        }
        
        // Check for excessive dimensions (would cause memory exhaustion)
        if (m_rows > MAX_ROWS || m_columns > MAX_COLUMNS || m_levels > MAX_LEVELS) {

            return false;
        }
        
        // Check for potential overflow in total cell calculation
        constexpr auto max_cells = std::numeric_limits<size_t>::max() / sizeof(void*);

        if (static_cast<size_t>(m_rows) * m_columns * m_levels > max_cells) {
            return false;
        }
        
        return true;
    }

    /// @brief Reset all values to safe defaults
    /// @details Useful for clearing potentially corrupted configuration
    void reset_to_defaults() noexcept {

        m_rows = DEFAULT_ROWS;
        m_columns = DEFAULT_COLUMNS; 
        m_levels = DEFAULT_LEVELS;
        m_block_id = DEFAULT_BLOCK_ID;
        m_algo_id = DEFAULT_ALGO_ID;
        m_seed = DEFAULT_SEED;
        m_distances = DEFAULT_DISTANCES;
        m_output_id = DEFAULT_OUTPUT_ID;
        m_help.clear();
        m_version.clear();
    }

private:

    unsigned int m_rows;

    unsigned int m_columns;

    unsigned int m_levels;

    int m_block_id;

    algo m_algo_id;

    unsigned int m_seed;

    bool m_distances;

    output m_output_id;

    std::string m_help;

    std::string m_version;
};

} // namespace

#endif // CONFIGURATOR_H
