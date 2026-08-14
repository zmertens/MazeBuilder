#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <string_view>
#include <utility>
#include <vector>

#include <MazeBuilder/barriers.h>

#include <cstddef>

/// @file topology.h
/// @namespace mazes
namespace mazes
{
    /// @struct cell_walls
    /// @brief Per-cell wall presence, used by consumers that need a lightweight
    /// row/column topology (e.g. physics or wireframe rendering) instead of a full grid.
    struct cell_walls
    {
        [[nodiscard]] bool north() const noexcept { return north_wall.second; }
        [[nodiscard]] bool south() const noexcept { return south_wall.second; }
        [[nodiscard]] bool east() const noexcept { return east_wall.second; }
        [[nodiscard]] bool west() const noexcept { return west_wall.second; }

        void set_north(bool has_wall) noexcept { north_wall.second = has_wall; }
        void set_south(bool has_wall) noexcept { south_wall.second = has_wall; }
        void set_east(bool has_wall) noexcept { east_wall.second = has_wall; }
        void set_west(bool has_wall) noexcept { west_wall.second = has_wall; }

    private:
        std::pair<Direction, bool> north_wall{Direction::NORTH, true};
        std::pair<Direction, bool> south_wall{Direction::SOUTH, true};
        std::pair<Direction, bool> east_wall{Direction::EAST, true};
        std::pair<Direction, bool> west_wall{Direction::WEST, true};

        static constexpr std::size_t NUM_WALLS = 4u;
    };

    /// @class topology
    /// @brief Row/column wall topology parsed from an ASCII box-drawing maze grid
    class topology
    {
    public:
        unsigned int rows{0};
        unsigned int columns{0};
        std::vector<cell_walls> cells;

        /// @brief Get the walls for a cell, or nullptr if out of bounds
        /// @param row
        /// @param col
        /// @return
        [[nodiscard]] const cell_walls *at(unsigned int row, unsigned int col) const noexcept;

        /// @brief Parse an ASCII box-drawing maze grid (as produced by the stringify/text output format)
        /// into per-cell wall data
        /// @param txt
        /// @return An empty topology (rows == 0) if txt cannot be parsed
        [[nodiscard]] static topology parse(std::string_view txt) noexcept;
    };
} // namespace mazes

#endif // TOPOLOGY_H
