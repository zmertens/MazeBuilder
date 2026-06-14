#ifndef MASKED_GRID_H
#define MASKED_GRID_H

#include <MazeBuilder/grid.h>
#include <MazeBuilder/mask.h>
#include <MazeBuilder/randomizer.h>

#include <memory>

/// @file masked_grid.h
/// @namespace mazes
namespace mazes
{
    class cell;

    /// @class masked_grid
    /// @brief A grid that restricts cell availability based on a boolean mask
    /// @details Cells at positions where the mask returns false are treated as non-existent.
    ///          Inherits from grid and overrides search() so that algorithms naturally skip
    ///          blocked positions. Use binary_tree or sidewinder algorithms with masked grids.
    class masked_grid : public grid
    {
    public:
        // Non-copyable (contains a mask with non-trivial state)
        masked_grid(const masked_grid &) = delete;
        masked_grid &operator=(const masked_grid &) = delete;

        masked_grid(masked_grid &&) noexcept = default;
        masked_grid &operator=(masked_grid &&) noexcept = default;

        /// @brief Construct a masked grid from a mask object
        /// @param m The mask controlling which cells are available
        explicit masked_grid(mask m);

        ~masked_grid() override = default;

        /// @brief Search for a cell by index, respecting the mask
        /// @param index Cell index (row * columns + col)
        /// @return The cell if the position is available, nullptr if blocked or out of range
        std::shared_ptr<cell> search(int index) const noexcept override;

        /// @brief Return the number of available (unmasked) cells
        /// @return Count of available cells as reported by the mask
        int num_cells() const noexcept override;

        /// @brief Access the mask used by this grid
        /// @return Const reference to the mask
        const mask &get_mask() const noexcept;

        /// @brief Return a random available cell based on the mask
        /// @param rng Randomizer to use
        /// @return A shared pointer to a random available cell
        std::shared_ptr<cell> random_cell(randomizer &rng) noexcept;

    private:
        mask m_mask;
    };

} // namespace mazes

#endif // MASKED_GRID_H
