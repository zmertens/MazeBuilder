#ifndef MASK_H
#define MASK_H

#include <MazeBuilder/randomizer.h>

#include <string>
#include <utility>
#include <vector>

namespace mazes
{

    /// @file mask.h
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

        mask(const mask &) = default;
        mask &operator=(const mask &) = default;

        mask(mask &&) noexcept = default;
        mask &operator=(mask &&) noexcept = default;

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
        std::pair<unsigned int, unsigned int> random_location(randomizer &rng) const noexcept;

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
        static mask from_txt(const std::string &filename);

    private:
        unsigned int m_rows;
        unsigned int m_columns;
        std::vector<std::vector<bool>> m_bits;
    };

} // namespace mazes

#endif // MASK_H
