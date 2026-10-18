#ifndef MASKED_GRID_H
#define MASKED_GRID_H

#include <MazeBuilder/grid.h>
#include <MazeBuilder/mask.h>

#include <memory>

/// @file masked_grid.h
/// @namespace mazes
namespace mazes
{
    class cell;
    class randomizer;

    /// @class masked_grid
    /// @brief A grid that restricts cell availability based on a boolean mask
    /// @details Cells at positions where the mask returns false are treated as non-existent.
    ///          Inherits from grid and overrides search() so that algorithms naturally skip
    ///          blocked positions. Use binary_tree or sidewinder algorithms with masked grids.
    class masked_grid : public grid
    {
    public:
        /// @brief Constructs a masked grid with specified dimensions.
        /// @param width The width of the grid. Defaults to 1.
        /// @param length The length of the grid. Defaults to 1.
        /// @param height The number of levels in the grid. Defaults to 1.
        explicit masked_grid(
            unsigned int width = 1u,
            unsigned int length = 1u,
            unsigned int height = 1u
        );

        /// @brief Constructs a masked grid sized to the given mask
        /// @param m The mask restricting available cells
        explicit masked_grid(mask m);

        ~masked_grid() override = default;

        masked_grid(const masked_grid &) = delete;
        masked_grid &operator=(const masked_grid &) = delete;

        masked_grid(masked_grid &&) noexcept = default;
        masked_grid &operator=(masked_grid &&) noexcept = default;

        /// @brief Get the number of available cells in the masked grid
        /// @return The count of cells that are not blocked by the mask
        [[nodiscard]] std::size_t num_cells() const noexcept override;
        
        /// @brief Search for a cell by index, respecting the mask
        /// @param index Cell index (row * length + col)
        /// @return The cell if the position is available, nullptr if blocked or out of range
        std::shared_ptr<cell> search(int index) const noexcept override;

        /// @brief Access the mask used by this grid
        /// @return Const reference to the mask
        const mask &get_mask() const noexcept;

        /// @brief Return a random available cell based on the mask
        /// @param rng Randomizer to use
        /// @return A shared pointer to a random available cell
        std::shared_ptr<cell> random_cell(randomizer &rng) const noexcept;

    private:
        mask m_mask;
    };
} // namespace mazes

#endif // MASKED_GRID_H
