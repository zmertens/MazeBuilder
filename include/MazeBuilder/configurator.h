#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

#include <MazeBuilder/algos.h>
#include <MazeBuilder/args.h>
#include <MazeBuilder/convert_contract.h>
#include <MazeBuilder/output_formats.h>

#include <algorithm>
#include <limits>
#include <memory>
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
        configurator &ensure_rows(unsigned int rows) noexcept
        {
            // Clamp to reasonable limits to prevent infinite loops and memory issues
            m_rows = std::clamp(rows, 1u, MAX_ROWS);
            return *this;
        }

        /// @brief Set the number of columns
        /// @param columns The number of columns (must be > 0, will be clamped to reasonable limits)
        /// @return A reference to this configurator
        /// @warning Values will be clamped to prevent memory issues
        configurator &ensure_columns(unsigned int columns) noexcept
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
        configurator &ensure_levels(unsigned int levels) noexcept
        {
            // Clamp to reasonable limits to prevent infinite loops and memory issues
            // Levels are more memory-intensive than rows/columns, so lower limit
            m_levels = std::clamp(levels, 1u, MAX_LEVELS);
            return *this;
        }

        /// @brief Set the maze generation algorithm
        /// @param algorithm The algorithm to use
        /// @return A reference to this configurator
        configurator &ensure_algo_id(algo algorithm) noexcept
        {
            m_algo_id = algorithm;
            return *this;
        }

        /// @brief Set the block ID
        /// @param block_id The block ID
        /// @return A reference to this configurator
        configurator &ensure_block_id(int block_id) noexcept
        {
            m_block_id = block_id;
            return *this;
        }

        /// @brief Set the random seed
        /// @param seed The random seed (0 = use random seed)
        /// @return A reference to this configurator
        configurator &ensure_seed(unsigned int seed) noexcept
        {
            m_seed = seed;
            return *this;
        }

        /// @brief Set the distance calculation flag
        /// @param distances The distance calculation flag
        /// @return A reference to this configurator
        configurator &ensure_distances(bool distances) noexcept
        {
            m_distances = distances;
            return *this;
        }

        /// @brief Set the distance start index
        /// @param start_index The starting cell index for distance calculation
        /// @return A reference to this configurator
        configurator &ensure_distances_start(int start_index) noexcept
        {
            m_distances_start = start_index;
            return *this;
        }

        /// @brief Set the distance end index
        /// @param end_index The ending cell index for distance calculation
        /// @return A reference to this configurator
        configurator &ensure_distances_end(int end_index) noexcept
        {
            m_distances_end = end_index;
            return *this;
        }

        /// @brief Set the output_format ID
        /// @param output_format The output_format ID
        /// @return A reference to this configurator
        configurator &ensure_output_format_id(output_format output_format) noexcept
        {
            m_output_format_id = output_format;
            return *this;
        }

        /// @brief Set the output_format filename
        /// @param filename The output_format filename
        /// @return A reference to this configurator
        configurator &ensure_output_format_filename(std::string filename) noexcept
        {
            m_output_filename = std::move(filename);
            return *this;
        }

        /// @brief Set the mask filename
        /// @param filename The mask file path (.txt)
        /// @return A reference to this configurator
        configurator &ensure_mask_filename(std::string filename) noexcept
        {
            m_mask_filename = std::move(filename);
            return *this;
        }

        /// @brief Set the help flag
        /// @param help
        /// @return
        configurator &needs_help(bool help) noexcept
        {
            m_help = help;
            return *this;
        };

        /// @brief Set the version flag
        /// @param version
        /// @return
        configurator &needs_version(bool version) noexcept
        {
            m_version = version;
            return *this;
        };

        /// @brief Set whether step snapshots should be shown during generation
        /// @param show_steps True to emit periodic maze snapshots
        /// @return A reference to this configurator
        configurator &show_steps(bool show_steps) noexcept
        {
            m_show_steps = show_steps;
            return *this;
        }

        // Shorthand setter overloads (non-const, take a value; getters are const with no params)
        configurator &rows(unsigned int r) noexcept { return ensure_rows(r); }
        configurator &columns(unsigned int c) noexcept { return ensure_columns(c); }
        configurator &levels(unsigned int l) noexcept { return ensure_levels(l); }
        configurator &algo_id(algo a) noexcept { return ensure_algo_id(a); }
        configurator &seed(unsigned int s) noexcept { return ensure_seed(s); }

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

        /// @brief Get the mask filename
        /// @return The mask filename (empty string if not set)
        [[nodiscard]] std::string mask_filename() const noexcept { return m_mask_filename.value_or(std::string{}); }

        /// @brief Get the block ID
        /// @return The block ID
        [[nodiscard]] int block_ID() const noexcept { return m_block_id.value_or(0); }

        /// @brief Get the random seed
        /// @return The random seed
        [[nodiscard]] unsigned int seed() const noexcept { return m_seed.value_or(DEFAULT_SEED_VALUE); }

        /// @brief Check if distances are calculated
        /// @return True if distances are calculated, false otherwise
        [[nodiscard]] bool distances() const noexcept { return m_distances.value_or(false); }

        /// @brief Get the distance start index
        /// @return The starting cell index for distance calculation
        [[nodiscard]] int distances_start() const noexcept { return m_distances_start.value_or(DEFAULT_DISTANCES_START); }

        /// @brief Get the distance end index
        /// @return The ending cell index for distance calculation
        [[nodiscard]] int distances_end() const noexcept { return m_distances_end.value_or(DEFAULT_DISTANCES_END); }

        /// @brief Get the output_format ID
        /// @return The output_format ID
        [[nodiscard]] output_format output_format_id() const noexcept { return m_output_format_id.value_or(output_format::STDOUT); }

        /// @brief Get the output_format filename
        /// @return The output_format filename
        [[nodiscard]] std::string config_filename() const noexcept { return m_output_filename.value_or(std::string{"config.json"}); }

        [[nodiscard]] bool help() const noexcept { return m_help.value_or(false); };

        [[nodiscard]] bool version() const noexcept { return m_version.value_or(false); };

        [[nodiscard]] bool show_steps() const noexcept { return m_show_steps.value_or(false); };

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
            if (constexpr auto max_cells = std::numeric_limits<size_t>::max() / sizeof(void *);
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

        std::optional<int> m_block_id;

        std::optional<unsigned int> m_seed;

        std::optional<bool> m_distances;

        std::optional<int> m_distances_start;

        std::optional<int> m_distances_end;

        std::optional<output_format> m_output_format_id;

        std::optional<std::string> m_config_file;

        std::optional<std::string> m_mask_filename;

        std::optional<std::string> m_output_filename;

        std::optional<bool> m_help;

        std::optional<bool> m_version;

        std::optional<bool> m_show_steps;
    };

} // namespace

#endif // CONFIGURATOR_H
