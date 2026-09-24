#ifndef MASK_H
#define MASK_H

#include <MazeBuilder/randomizer.h>

#include <string>
#include <string_view>
#include <utility>
#include <vector>

/// @file mask.h
/// @namespace mazes
namespace mazes
{
    /// @class mask
    /// @brief 2D boolean mask for controlling which cells are available in maze generation
    /// @details Cells marked as blocked (false) are excluded from maze generation.
    ///          "X" characters in text files denote blocked cells.
    class mask
    {
    public:
        /// @brief Construct a mask with specified dimensions, all cells enabled
        /// @param rows Number of rows
        /// @param columns Number of columns
        explicit mask(unsigned int rows, unsigned int columns);

        ~mask() = default;

        mask(const mask&) = default;
        mask& operator=(const mask&) = default;

        mask(mask&&) noexcept = default;
        mask& operator=(mask&&) noexcept = default;

        /// @brief Check if a cell at (row, column) is available
        /// @param row Row index
        /// @param column Column index
        /// @return true if available, false if blocked or out of bounds
        bool operator()(unsigned int row, unsigned int column) const noexcept;

        /// @brief Set a cell's availability
        /// @param row Row index
        /// @param column Column index
        /// @param is_on true to enable, false to block
        void set(unsigned int row, unsigned int column, bool is_on) noexcept;

        /// @brief Count the number of available cells
        /// @return Number of available cells
        int count() const noexcept;

        /// @brief Get a random available cell location
        /// @param rng Randomizer to use
        /// @return A pair (row, col) of a random available cell
        /// @pre count() > 0 — callers must ensure at least one cell is available
        std::pair<unsigned int, unsigned int> random_location(randomizer& rng) const noexcept;

        /// @brief Get the number of rows
        /// @return Number of rows
        [[nodiscard]] unsigned int rows() const noexcept { return m_rows; }

        /// @brief Get the number of columns
        /// @return Number of columns
        [[nodiscard]] unsigned int columns() const noexcept { return m_columns; }

        /// @brief Load a mask from a text file
        /// @param filename Path to the text file ('X' = blocked, other = available)
        /// @return A mask loaded from the file
        /// @throws std::runtime_error if the file cannot be opened or is empty
        /// @note Lines of different lengths are supported; shorter rows default to available
        static mask from_txt(const std::string& filename);

        /// @brief Load a mask from an inline string
        /// @param text Mask text ('X' = blocked, other = available)
        /// @return A mask loaded from the string
        /// @throws std::runtime_error if the string is empty or invalid
        /// @note Supports embedded newlines and escaped newline sequences such as "\n"
        static mask from_string(std::string_view text);

        /// @brief Load a mask from either a file path or an inline string
        /// @param source File path or inline mask string
        /// @return A mask loaded from the given source
        /// @throws std::runtime_error if the source cannot be resolved
        static mask from_source(std::string_view source);

    private:
        unsigned int m_rows;
        unsigned int m_columns;
        std::vector<std::vector<bool>> m_bits;
    };
} // namespace mazes

#endif // MASK_H
