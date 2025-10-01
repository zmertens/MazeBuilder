#ifndef DISTANCE_GRID_H
#define DISTANCE_GRID_H

#include <MazeBuilder/grid_interface.h>

#include <future>
#include <memory>
#include <string>
#include <unordered_map>

namespace mazes
{

    class cell;
    class distances;
    class grid;
    class grid_operations;

    /// @file distance_grid.h
    /// @class distance_grid
    /// @brief A grid that can calculate distances between cells
    class distance_grid : public grid_interface
    {
    public:
        explicit distance_grid(unsigned int width = 1u, unsigned int length = 1u, unsigned int levels = 1u);

        distance_grid(const distance_grid &) = delete;

        distance_grid &operator=(const distance_grid &) = delete;

        distance_grid(distance_grid &&) noexcept = default;

        distance_grid &operator=(distance_grid &&) noexcept = default;

        /// @brief
        /// @param indices
        /// @return
        // void configure(const std::vector<int>& indices) noexcept override;

        /// @brief
        /// @param c
        /// @return
        virtual std::string contents_of(std::shared_ptr<cell> const &c) const noexcept override;

        /// @brief
        /// @param c
        /// @return
        virtual std::uint32_t background_color_for(std::shared_ptr<cell> const &c) const noexcept override;

        // Delegate to embedded grid
        virtual grid_operations &operations() noexcept override;

        virtual const grid_operations &operations() const noexcept override;

        /// @brief Calculates distances for a range of indices.
        /// @param start_index The starting index of the range (inclusive).
        /// @param end_index The ending index of the range (exclusive).
        void calculate_distances(int start_index, int end_index) noexcept;

        std::shared_ptr<distances> get_distances() const noexcept;

    private:
        std::string to_base36(int value) const;

        std::shared_ptr<distances> m_distances;

        std::unique_ptr<grid_interface> m_grid;
    };
}

#endif // DISTANCE_GRID_H
