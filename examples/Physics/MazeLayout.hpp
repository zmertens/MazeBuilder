#ifndef MAZE_LAYOUT_HPP
#define MAZE_LAYOUT_HPP

#include <cstdint>
#include <string_view>
#include <vector>

#include <MazeBuilder/enums.h>

struct SDL_Surface;

class MazeLayout
{
public:
    enum class CellType : std::uint8_t
    {
        Empty,
        Wall,
        Other
    };

    enum class BarrierType : std::uint8_t
    {
        None,
        Corner,
        Horizontal,
        Vertical
    };

    struct Cell
    {
        CellType type{CellType::Empty};
        BarrierType barrier{BarrierType::None};
        std::uint8_t r{255};
        std::uint8_t g{255};
        std::uint8_t b{255};
        std::uint8_t a{255};
    };

    MazeLayout() = default;

    static MazeLayout fromString(std::string_view mazeStr, int cellSize);

    [[nodiscard]] int getRows() const noexcept { return mRows; }
    [[nodiscard]] int getColumns() const noexcept { return mColumns; }
    [[nodiscard]] int getCellSize() const noexcept { return mCellSize; }

    [[nodiscard]] int getPixelWidth() const noexcept { return mColumns * mCellSize; }
    [[nodiscard]] int getPixelHeight() const noexcept { return mRows * mCellSize; }

    [[nodiscard]] const Cell& at(const int row, const int col) const noexcept
    {
        return mCells[static_cast<std::size_t>(row) * mColumns + col];
    }

    [[nodiscard]] SDL_Surface* buildSurface() const noexcept;

private:
    int mRows{0};
    int mColumns{0};
    int mCellSize{0};
    std::vector<Cell> mCells;
};

#endif // MAZE_LAYOUT_HPP
