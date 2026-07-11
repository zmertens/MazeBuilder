#ifndef COLORED_GRID_H
#define COLORED_GRID_H

#include <MazeBuilder/grid_interface.h>

/// @file colored_grid.h
/// @namespace mazes
namespace mazes
{
    class cell;
    class distances;
    class grid_operations;

    /// @brief A colored grid implementation that extends the grid_interface to include distance-based coloring
    class colored_grid : public grid_interface
    {
    public:
        // Delete copy constructor and copy assignment operator to fix the static assertion failure
        colored_grid(const colored_grid&) = delete;
        colored_grid& operator=(const colored_grid&) = delete;

        // Explicitly define move constructor and move assignment operator
        colored_grid(colored_grid&&) noexcept = default;
        colored_grid& operator=(colored_grid&&) noexcept = default;

        /// @brief Constructs a colored grid with specified dimensions.
        /// @param width The width of the grid. Defaults to 1.
        /// @param length The length of the grid. Defaults to 1.
        /// @param levels The number of levels in the grid. Defaults to 1.
        explicit colored_grid(unsigned int width = 1u, unsigned int length = 1u, unsigned int levels = 1u);

        /// @brief Retrieves the contents of a given cell, if available.
        /// @param c A shared pointer to the cell whose contents are to be retrieved.
        /// @return If the cell has no contents, the contents are considered empty
        [[nodiscard]] std::string contents_of(const std::shared_ptr<cell>& c) const noexcept override;

        /// @brief Retrieves the background color for a given cell, if available.
        /// @param c A shared pointer to the cell for which the background color is to be retrieved.
        /// @return An 32-bit unsigned integer containing the background color
        [[nodiscard]] std::uint32_t background_color_for(const std::shared_ptr<cell>& c) const noexcept override;

        /// @brief Initialize distance coloring from a starting cell index.
        /// @param start_index The index of the starting cell for distance calculation.
        /// @param goal_index The index of the goal cell for distance calculation.
        void initialize_distance_coloring(int start_index, int goal_index) noexcept;

        // Delegate to the embedded grid
        [[nodiscard]] grid_operations& operations() noexcept override;

        // Delegate to the embedded grid
        [[nodiscard]] const grid_operations& operations() const noexcept override;

        /// @brief Resize the grid to the specified dimensions.
        /// @param rows
        /// @param cols
        /// @param levels
        void resize(unsigned int rows, unsigned int cols, unsigned int levels) const noexcept;

    private:
        std::shared_ptr<distances> m_distances;
        std::unique_ptr<grid_interface> m_grid;
    };
} // namespace mazes

#endif // COLORED_GRID_H
