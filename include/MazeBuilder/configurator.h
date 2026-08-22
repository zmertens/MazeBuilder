#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/output_formats.h>

#include <algorithm>
#include <limits>
#include <optional>
#include <string>

/// @namespace mazes
/// @file configurator.h
namespace mazes
{
    /// @class configurator
    /// @brief Configuration class for arguments
    /// @details This class stores maze generation parameters with safe default values
    class configurator final
    {
    public:
        static constexpr auto MAX_COLUMNS = 100u;
        static constexpr auto MAX_LEVELS = 10u;
        static constexpr auto MAX_ROWS = 100u;
        static constexpr auto DEFAULT_DISTANCES_START = 0;
        static constexpr auto DEFAULT_DISTANCES_END = -1;
        static constexpr auto DEFAULT_SEED_VALUE = 1'000'000u;

        /// @brief Set the number of rows
        /// @param rows The number of rows (must be > 0, will be clamped to reasonable limits)
        /// @return A reference to this configurator
        /// @warning Values will be clamped to prevent memory issues
        configurator& ensure_rows(const unsigned int rows) noexcept
        {
            // Clamp to reasonable limits to prevent infinite loops and memory issues
            m_rows = std::clamp(rows, 1u, MAX_ROWS);
            return *this;
        }

        /// @brief Set the number of columns
        /// @param columns The number of columns (must be > 0, will be clamped to reasonable limits)
        /// @return A reference to this configurator
        /// @warning Values will be clamped to prevent memory issues
        configurator& ensure_columns(const unsigned int columns) noexcept
        {
            // Clamp to reasonable limits to prevent infinite loops and memory issues
            m_columns = std::clamp(columns, 1u, MAX_COLUMNS);
            return *this;
        }

        /// @brief Set the number of levels
        /// @param levels The number of levels (must be > 0, will be clamped to reasonable limits)
        /// @return A reference to this configurator
        /// @warning Values will be clamped to prevent memory issues
        /// @note Most mazes are 2D (levels=1), 3D mazes should use moderate level counts
        configurator& ensure_levels(const unsigned int levels) noexcept
        {
            // Clamp to reasonable limits to prevent infinite loops and memory issues
            // Levels are more memory-intensive than rows/columns, so lower limit
            m_levels = std::clamp(levels, 1u, MAX_LEVELS);
            return *this;
        }

        /// @brief Set the maze generation algorithm
        /// @param algorithm The algorithm to use
        /// @return A reference to this configurator
        configurator& ensure_algo_id(const algo algorithm) noexcept
        {
            m_algo_id = algorithm;
            return *this;
        }

        /// @brief Set the random seed
        /// @param seed The random seed
        /// @return A reference to this configurator
        configurator& ensure_seed(const unsigned int seed) noexcept
        {
            m_seed = seed;
            return *this;
        }

        /// @brief Set the distance calculation flag
        /// @param distances The distance calculation flag
        /// @return A reference to this configurator
        configurator& ensure_distances(const bool distances) noexcept
        {
            m_distances = distances;
            return *this;
        }

        /// @brief Set the distance start index
        /// @param start_index The starting cell index for distance calculation
        /// @return A reference to this configurator
        configurator& ensure_distances_start(const int start_index) noexcept
        {
            m_distances_start = start_index;
            return *this;
        }

        /// @brief Set the distance end index
        /// @param end_index The ending cell index for distance calculation
        /// @return A reference to this configurator
        configurator& ensure_distances_end(const int end_index) noexcept
        {
            m_distances_end = end_index;
            return *this;
        }

        // Shorthand setter overloads (non-const, take a value; getters are const with no params)
        configurator& rows(const unsigned int r) noexcept { return ensure_rows(r); }
        configurator& columns(const unsigned int c) noexcept { return ensure_columns(c); }
        configurator& levels(const unsigned int l) noexcept { return ensure_levels(l); }
        configurator& algo_id(const algo a) noexcept { return ensure_algo_id(a); }
        configurator& seed(const unsigned int s) noexcept { return ensure_seed(s); }

        /// @brief Get the number of rows
        /// @return The number of rows (guaranteed to be > 0)
        [[nodiscard]] unsigned int rows() const noexcept { return m_rows.value_or(MAX_ROWS); }

        /// @brief Get the number of columns
        /// @return The number of columns (guaranteed to be > 0)
        [[nodiscard]] unsigned int columns() const noexcept { return m_columns.value_or(MAX_COLUMNS); }

        /// @brief Get the number of levels
        /// @return The number of levels (guaranteed to be > 0)
        [[nodiscard]] unsigned int levels() const noexcept { return m_levels.value_or(MAX_LEVELS); }

        /// @brief Get the maze generation algorithm
        /// @return The algorithm used for maze generation
        [[nodiscard]] algo algo_id() const noexcept { return m_algo_id.value_or(static_cast<algo>(0)); }


        /// @brief Get the random seed
        /// @return The random seed
        [[nodiscard]] unsigned int seed() const noexcept { return m_seed.value_or(DEFAULT_SEED_VALUE); }

        /// @brief Check if distances are calculated
        /// @return True if distances are calculated, false otherwise
        [[nodiscard]] bool distances() const noexcept { return m_distances.value_or(false); }

        /// @brief Get the distance start index
        /// @return The starting cell index for distance calculation
        [[nodiscard]] int distances_start() const noexcept
        {
            return m_distances_start.value_or(DEFAULT_DISTANCES_START);
        }

        /// @brief Get the distance end index
        /// @return The ending cell index for distance calculation
        [[nodiscard]] int distances_end() const noexcept { return m_distances_end.value_or(DEFAULT_DISTANCES_END); }

        /// @brief Set the mask file path
        /// @param mask_file_path Path to the mask file
        /// @return A reference to this configurator
        configurator& ensure_mask_file(const std::string& mask_file_path) noexcept
        {
            m_mask_file = mask_file_path;
            return *this;
        }

        /// @brief Get the mask file path
        /// @return The mask file path, or empty string if not set
        [[nodiscard]] std::string mask_file() const noexcept
        {
            return m_mask_file.value_or("");
        }

        /// @brief Validate all configuration values are within safe limits
        /// @return True if all values are valid, false if any are problematic
        /// @details Checks for potential infinite loop conditions and memory issues
        [[nodiscard]] bool has_valid_dimensions() const noexcept
        {
            // Check for zero dimensions (would cause infinite loops or divisions by zero)
            if (!m_rows.has_value() || !m_columns.has_value() || !m_levels.has_value())
            {
                return false;
            }

            // Check for excessive dimensions (would cause memory exhaustion)
            if (m_rows.value() > MAX_ROWS || m_columns.value() > MAX_COLUMNS || m_levels.value() > MAX_LEVELS)
            {
                return false;
            }

            // Check for potential overflow in total cell calculation
            if (constexpr auto max_cells = std::numeric_limits<size_t>::max() / sizeof(void*);
                static_cast<size_t>(m_rows.value()) * m_columns.value() * m_levels.value() > max_cells)
            {
                // Potential overflow detected
                return false;
            }

            return true;
        }

    private:
        std::optional<unsigned int> m_rows;

        std::optional<unsigned int> m_columns;

        std::optional<unsigned int> m_levels;

        std::optional<algo> m_algo_id;

        std::optional<unsigned int> m_seed;

        std::optional<bool> m_distances;

        std::optional<int> m_distances_start;

        std::optional<int> m_distances_end;

        std::optional<std::string> m_mask_file;
    };
} // namespace

#endif // CONFIGURATOR_H
