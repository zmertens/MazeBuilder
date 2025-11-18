#include "MazeLayout.hpp"

#include <SDL3/SDL.h>

MazeLayout MazeLayout::fromString(std::string_view mazeStr, int cellSize)
{
    MazeLayout layout;

    if (mazeStr.empty() || cellSize <= 0)
    {
        return layout;
    }

    int currentLineWidth = 0;
    int rows = 0;
    int maxWidth = 0;

    for (char c : mazeStr)
    {
        if (c == '\n')
        {
            ++rows;
            if (currentLineWidth > maxWidth)
            {
                maxWidth = currentLineWidth;
            }
            currentLineWidth = 0;
        }
        else
        {
            ++currentLineWidth;
        }
    }

    if (currentLineWidth > 0)
    {
        ++rows;
        if (currentLineWidth > maxWidth)
        {
            maxWidth = currentLineWidth;
        }
    }

    if (rows == 0 || maxWidth == 0)
    {
        return layout;
    }

    layout.mRows = rows;
    layout.mColumns = maxWidth;
    layout.mCellSize = cellSize;
    layout.mCells.resize(static_cast<std::size_t>(rows * maxWidth));

    int row = 0;
    int col = 0;

    for (char c : mazeStr)
    {
        if (c == '\n')
        {
            ++row;
            col = 0;
            continue;
        }

        if (row >= rows || col >= maxWidth)
        {
            ++col;
            continue;
        }

        std::size_t index = static_cast<std::size_t>(row) * maxWidth + col;
        Cell& cell = layout.mCells[index];

        // Default to transparent for empty cells
        cell.r = 0;
        cell.g = 0;
        cell.b = 0;
        cell.a = 0;

        if (c == static_cast<char>(mazes::barriers::CORNER))
        {
            cell.type = CellType::Wall;
            cell.barrier = BarrierType::Corner;
            cell.r = cell.g = cell.b = 0;
            cell.a = 255; // Opaque black for walls
        }
        else if (c == static_cast<char>(mazes::barriers::HORIZONTAL))
        {
            cell.type = CellType::Wall;
            cell.barrier = BarrierType::Horizontal;
            cell.r = cell.g = cell.b = 0;
            cell.a = 255; // Opaque black for walls
        }
        else if (c == static_cast<char>(mazes::barriers::VERTICAL))
        {
            cell.type = CellType::Wall;
            cell.barrier = BarrierType::Vertical;
            cell.r = cell.g = cell.b = 0;
            cell.a = 255; // Opaque black for walls
        }
        else if (c == ' ')
        {
            cell.type = CellType::Empty;
            cell.barrier = BarrierType::None;
            // Already transparent from default
        }
        else
        {
            cell.type = CellType::Other;
            cell.barrier = BarrierType::None;
            // Keep transparent for other/unknown cell types
        }

        ++col;
    }

    return layout;
}

SDL_Surface* MazeLayout::buildSurface() const noexcept
{
    if (mRows <= 0 || mColumns <= 0 || mCellSize <= 0)
    {
        return nullptr;
    }

    const int width = getPixelWidth();
    const int height = getPixelHeight();

    SDL_Surface* surface = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA8888);

    if (!surface)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create maze surface: %s", SDL_GetError());
        return nullptr;
    }

    // Fill with transparent background to enable blending with parallax layers
    SDL_FillSurfaceRect(surface, nullptr, SDL_MapSurfaceRGBA(surface, 0, 0, 0, 0));

    for (int row = 0; row < mRows; ++row)
    {
        for (int col = 0; col < mColumns; ++col)
        {
            const Cell& cell = at(row, col);

            SDL_Rect rect{col * mCellSize, row * mCellSize, mCellSize, mCellSize};

            SDL_FillSurfaceRect(surface, &rect, SDL_MapSurfaceRGBA(surface, cell.r, cell.g, cell.b, cell.a));
        }
    }

    return surface;
}
