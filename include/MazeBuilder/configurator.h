#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

#include <limits>
#include <optional>
#include <string>

#include <MazeBuilder/enums.h>

namespace mazes {

/// @file configurator.h
/// @class configurator
/// @brief Configuration class for arguments
/// @details This class stores maze generation parameters with safe default values
class configurator {
    
public:

    static constexpr auto DEFAULT_ROWS = 10u;

    static constexpr auto DEFAULT_COLUMNS = 10u;

    static constexpr auto DEFAULT_LEVELS = 1u;

    static constexpr auto DEFAULT_BLOCK_ID = 0;

    static constexpr auto DEFAULT_ALGO_ID = algo::BINARY_TREE;

    static constexpr auto DEFAULT_SEED = 0u;

    static constexpr auto DEFAULT_OUTPUT_ID = output_format::PLAIN_TEXT;

    static constexpr auto DEFAULT_DISTANCES = false;

    static constexpr auto DEFAULT_DISTANCES_START = 0;

    static constexpr auto DEFAULT_DISTANCES_END = -1;

    static constexpr auto MAX_ROWS = 100u;

    static constexpr auto MAX_COLUMNS = 100u;

    static constexpr auto MAX_LEVELS = 10u;
    
    /// @brief Set the number of rows
    /// @param rows The number of rows (must be > 0, will be clamped to reasonable limits)
    /// @return A reference to this configurator
    /// @warning Values be clamped to prevent memory issues
    configurator& rows(unsigned int rows) noexcept {
        // Clamp to reasonable limits to prevent infinite loops and memory issues
        m_rows = (rows == 0) ? 1 : (rows > MAX_ROWS) ? MAX_ROWS : rows;
        return *this;
    }

    /// @brief Set the number of columns
    /// @param columns The number of columns (must be > 0, will be clamped to reasonable limits)
    /// @return A reference to this configurator
    /// @warning Values will be clamped to prevent memory issues
    configurator& columns(unsigned int columns) noexcept {
        // Clamp to reasonable limits to prevent infinite loops and memory issues
        m_columns = (columns == 0) ? 1 : (columns > MAX_COLUMNS) ? MAX_COLUMNS : columns;
        return *this;
    }

    /// @brief Set the number of levels
    /// @param levels The number of levels (must be > 0, will be clamped to reasonable limits)
    /// @return A reference to this configurator
    /// @warning Values will be clamped to prevent memory issues
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

    /// @brief Set the distance start index
    /// @param start_index The starting cell index for distance calculation
    /// @return A reference to this configurator
    configurator& distances_start(int start_index) noexcept {
        m_distances_start = start_index;
        return *this;
    }

    /// @brief Set the distance end index
    /// @param end_index The ending cell index for distance calculation
    /// @return A reference to this configurator
    configurator& distances_end(int end_index) noexcept {
        m_distances_end = end_index;
        return *this;
    }

    /// @brief Set the output_format ID
    /// @param output_format The output_format ID
    /// @return A reference to this configurator
    configurator& output_format_id(output_format output_format) noexcept {
        m_output_format_id = output_format;
        return *this;
    }

    /// @brief Set the output_format filename
    /// @param filename The output_format filename
    /// @return A reference to this configurator
    configurator& output_format_filename(std::string filename) noexcept {
        m_output_format_filename = std::move(filename);
        return *this;
    }

    /// @brief Get the number of rows
    /// @return The number of rows (guaranteed to be > 0)
    unsigned int rows() const noexcept { return m_rows.value_or(DEFAULT_ROWS); }

    /// @brief Get the number of columns  
    /// @return The number of columns (guaranteed to be > 0)
    unsigned int columns() const noexcept { return m_columns.value_or(DEFAULT_COLUMNS); }

    /// @brief Get the number of levels
    /// @return The number of levels (guaranteed to be > 0)
    unsigned int levels() const noexcept { return m_levels.value_or(DEFAULT_LEVELS); }

    /// @brief Get the maze generation algorithm
    /// @return The algorithm used for maze generation
    algo algo_id() const noexcept { return m_algo_id.value_or(DEFAULT_ALGO_ID); }

    /// @brief Get the block ID
    /// @return The block ID
    int block_id() const noexcept { return m_block_id.value_or(DEFAULT_BLOCK_ID); }

    /// @brief Get the random seed
    /// @return The random seed
    unsigned int seed() const noexcept { return m_seed.value_or(DEFAULT_SEED); }

    /// @brief Check if distances are calculated
    /// @return True if distances are calculated, false otherwise
    bool distances() const noexcept { return m_distances.value_or(DEFAULT_DISTANCES); }

    /// @brief Get the distance start index
    /// @return The starting cell index for distance calculation
    int distances_start() const noexcept { return m_distances_start.value_or(DEFAULT_DISTANCES_START); }

    /// @brief Get the distance end index
    /// @return The ending cell index for distance calculation
    int distances_end() const noexcept { return m_distances_end.value_or(DEFAULT_DISTANCES_END); }

    /// @brief Get the output_format ID
    /// @return The output_format ID
    output_format output_format_id() const noexcept { return m_output_format_id.value_or(DEFAULT_OUTPUT_ID); }

    /// @brief Get the output_format filename
    /// @return The output_format filename
    const std::string& output_format_filename() const noexcept { return m_output_format_filename.value_or("output.txt"); }

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

        if (static_cast<size_t>(m_rows.value()) * m_columns.value() * m_levels.value() > max_cells) {
            // Potential overflow detected
            return false;
        }
        
        return true;
    }

private:

    std::optional<unsigned int> m_rows;

    std::optional<unsigned int> m_columns;

    std::optional<unsigned int> m_levels;

    std::optional<int> m_block_id;

    std::optional<algo> m_algo_id;

    std::optional<unsigned int> m_seed;

    std::optional<bool> m_distances;

    std::optional<int> m_distances_start;

    std::optional<int> m_distances_end;

    std::optional<output_format> m_output_format_id;

    std::optional<std::string> m_output_format_filename;
};

} // namespace

#endif // CONFIGURATOR_H
