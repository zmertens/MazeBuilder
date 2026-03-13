/// @file main.cpp
/// @brief AmazingSFML - 2D maze generation and visualization example
/// @details Generates a 2D maze using the MazeBuilder library and visualizes it
///          using the SFML Graphics module.  Walls are drawn as the window background
///          colour and passages are carved out as lighter filled rectangles.
///
/// Controls:
///   Escape / close window  → exit

#include <SFML/Graphics.hpp>

#include <memory>

#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/colored_grid.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/randomizer.h>

int main()
{
    // ------------------------------------------------------------------ //
    // Maze parameters
    // ------------------------------------------------------------------ //
    constexpr unsigned int MAZE_ROWS = 20u;
    constexpr unsigned int MAZE_COLS = 20u;

    // Visual dimensions (pixels)
    // interior of each maze cell
    constexpr float CELL_SIZE = 24.f;
    // thickness of each wall
    constexpr float WALL_SIZE = 4.f;

    // ------------------------------------------------------------------ //
    // Generate the maze
    // ------------------------------------------------------------------ //
    auto g = std::make_unique<mazes::colored_grid>(MAZE_ROWS, MAZE_COLS, 1u);

    mazes::randomizer rng{};
    rng.seed(rng(0u, 4'200'000u));

    mazes::dfs dfs_algo{};
    dfs_algo.run(g.get(), rng);

    // Initialize distance-based coloring from the first cell (index 0)
    g->initialize_distance_coloring(0, g->operations().num_cells() - 1);

    // ------------------------------------------------------------------ //
    // Build a static vertex array for the entire maze
    // ------------------------------------------------------------------ //
    // Strategy: fill the window with the wall colour, then "carve" passages
    // by drawing cell interiors and openings in a lighter colour.
    const sf::Color wall_color(40, 40, 60);

    sf::VertexArray vertices(sf::PrimitiveType::Triangles);

    // Helper: extract RGB components from packed 24-bit color (0xRRGGBB format)
    auto color_from_uint32 = [](const std::uint32_t packed) -> sf::Color
    {
        const auto r = static_cast<std::uint8_t>((packed >> 16) & 0xFF);
        const auto g = static_cast<std::uint8_t>((packed >> 8) & 0xFF);
        const auto b = static_cast<std::uint8_t>(packed & 0xFF);
        return sf::Color(r, g, b);
    };

    // Helper: push an axis-aligned filled rectangle as two triangles
    auto push_rect = [&vertices](float x, float y, float w, float h, sf::Color color)
    {
        sf::Vertex topLeft, topRight, bottomLeft, bottomRight;
        topLeft.position = {x,     y    };
        topLeft.color = color;
        topRight.position = {x + w, y    };
        topRight.color = color;
        bottomLeft.position = {x,     y + h};
        bottomLeft.color = color;
        bottomRight.position = {x + w, y + h};
        bottomRight.color = color;

        vertices.append(topLeft);
        vertices.append(topRight);
        vertices.append(bottomLeft);
        vertices.append(topRight);
        vertices.append(bottomRight);
        vertices.append(bottomLeft);
    };

    for (unsigned int row = 0u; row < MAZE_ROWS; ++row)
    {
        for (unsigned int col = 0u; col < MAZE_COLS; ++col)
        {
            // Top-left pixel corner of this cell's interior
            const float cx = static_cast<float>(col) * (CELL_SIZE + WALL_SIZE) + WALL_SIZE;
            const float cy = static_cast<float>(row) * (CELL_SIZE + WALL_SIZE) + WALL_SIZE;

            // Cell interior color
            auto cell_color = g->background_color_for(g->operations().search(static_cast<int>(row * MAZE_COLS + col)));
            push_rect(cx, cy, CELL_SIZE, CELL_SIZE, color_from_uint32(cell_color));

            auto&& grid_ops = g->operations();
            const auto cell_ptr = grid_ops.search(static_cast<int>(row * MAZE_COLS + col));
            if (!cell_ptr)
            {
                continue;
            }

            // Open east passage when this cell is linked to its eastern neighbour
            if (const auto east = grid_ops.get_east(cell_ptr); east && cell_ptr->is_linked(east))
            {
                auto east_color = g->background_color_for(east);
                push_rect(cx + CELL_SIZE, cy, WALL_SIZE, CELL_SIZE, color_from_uint32(east_color));
            }

            // Open south passage when this cell is linked to its southern neighbour
            if (const auto south = grid_ops.get_south(cell_ptr); south && cell_ptr->is_linked(south))
            {
                auto south_color = g->background_color_for(south);
                push_rect(cx, cy + CELL_SIZE, CELL_SIZE, WALL_SIZE, color_from_uint32(south_color));
            }
        }
    }

    // ------------------------------------------------------------------ //
    // Create the SFML window
    // ------------------------------------------------------------------ //
    const auto WIN_WIDTH  = static_cast<unsigned int>(MAZE_COLS * (CELL_SIZE + WALL_SIZE) + WALL_SIZE);
    const auto WIN_HEIGHT = static_cast<unsigned int>(MAZE_ROWS * (CELL_SIZE + WALL_SIZE) + WALL_SIZE);

    sf::RenderWindow window(
        sf::VideoMode({WIN_WIDTH, WIN_HEIGHT}),
        "AmazingSFML - 2d mazes - " + mazes::buildinfo::Version
    );
    window.setFramerateLimit(60u);

    // ------------------------------------------------------------------ //
    // Main loop
    // ------------------------------------------------------------------ //
    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::Escape)
                {
                    window.close();
                }
            }
        }

        window.clear(wall_color);
        window.draw(vertices);
        window.display();
    }

    return 0;
}
