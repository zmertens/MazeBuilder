#ifndef COLORED_GRID_H
#define COLORED_GRID_H

#include <MazeBuilder/grid_interface.h>

namespace mazes
{

    class cell;
    class distances;
    class grid_operations;

    class colored_grid : public grid_interface
    {

    public:
        // Delete copy constructor and copy assignment operator to fix the static assertion failure
        colored_grid(const colored_grid &) = delete;
        colored_grid &operator=(const colored_grid &) = delete;

        // Explicitly define move constructor and move assignment operator
        colored_grid(colored_grid &&) noexcept = default;
        colored_grid &operator=(colored_grid &&) noexcept = default;

        /// @brief Constructs a colored grid with specified dimensions.
        /// @param width The width of the grid. Defaults to 1.
        /// @param length The length of the grid. Defaults to 1.
        /// @param levels The number of levels in the grid. Defaults to 1.
        explicit colored_grid(unsigned int width = 1u, unsigned int length = 1u, unsigned int levels = 1u);

        /// @brief Retrieves the contents of a given cell, if available.
        /// @param c A shared pointer to the cell whose contents are to be retrieved.
        /// @return If the cell has no contents, the contents are considered empty
        virtual std::string contents_of(const std::shared_ptr<cell> &c) const noexcept override;

        /// @brief Retrieves the background color for a given cell, if available.
        /// @param c A shared pointer to the cell for which the background color is to be retrieved.
        /// @return An 32-bit unsigned integer containing the background color
        virtual std::uint32_t background_color_for(const std::shared_ptr<cell> &c) const noexcept override;

        // Delegate to embedded grid
        grid_operations &operations() noexcept override;

        const grid_operations &operations() const noexcept override;

    private:
        std::shared_ptr<distances> m_distances;

        // Change from grid_interface to grid since we need grid's implementation of operations()
        std::unique_ptr<grid_interface> m_grid;
    };

} // namespace mazes

#endif // COLORED_GRID_H
