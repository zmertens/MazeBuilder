/// @file main.cpp
/// @brief AmazingSFML - 2D maze generation and visualization example
/// @details Generates a 2D maze using the MazeBuilder library and visualizes it
///          using the SFML Graphics module.  Walls are drawn as the window background
///          colour and passages are carved out as lighter filled rectangles.
///
/// Controls:
///   Escape / close window  → exit

#include <SFML/Graphics.hpp>

#include <MazeBuilder/cell.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/dfs.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/randomizer.h>

int main()
{
    // ------------------------------------------------------------------ //
    // Maze parameters
    // ------------------------------------------------------------------ //
    constexpr unsigned int MAZE_ROWS = 20u;
    constexpr unsigned int MAZE_COLS = 20u;

    // Visual dimensions (pixels)
    constexpr float CELL_SIZE = 24.f;  // interior of each maze cell
    constexpr float WALL_SIZE = 4.f;   // thickness of each wall

    // ------------------------------------------------------------------ //
    // Generate the maze
    // ------------------------------------------------------------------ //
    mazes::grid g(MAZE_ROWS, MAZE_COLS, 1u);

    mazes::randomizer rng{};
    rng.seed(42u);  // fixed seed → reproducible maze

    mazes::dfs dfs_algo{};
    dfs_algo.run(&g, rng);

    // ------------------------------------------------------------------ //
    // Build a static vertex array for the entire maze
    // ------------------------------------------------------------------ //
    // Strategy: fill the window with the wall colour, then "carve" passages
    // by drawing cell interiors and openings in a lighter colour.
    const sf::Color wall_color(40, 40, 60);    // dark blue-gray
    const sf::Color cell_color(240, 230, 210); // warm off-white

    sf::VertexArray vertices(sf::PrimitiveType::Triangles);

    // Helper: push an axis-aligned filled rectangle as two triangles
    auto push_rect = [&vertices](float x, float y, float w, float h, sf::Color color)
    {
        sf::Vertex tl, tr, bl, br;
        tl.position = {x,     y    };  tl.color = color;
        tr.position = {x + w, y    };  tr.color = color;
        bl.position = {x,     y + h};  bl.color = color;
        br.position = {x + w, y + h};  br.color = color;

        vertices.append(tl); vertices.append(tr); vertices.append(bl);
        vertices.append(tr); vertices.append(br); vertices.append(bl);
    };

    for (unsigned int row = 0u; row < MAZE_ROWS; ++row)
    {
        for (unsigned int col = 0u; col < MAZE_COLS; ++col)
        {
            // Top-left pixel corner of this cell's interior
            const float cx = static_cast<float>(col) * (CELL_SIZE + WALL_SIZE) + WALL_SIZE;
            const float cy = static_cast<float>(row) * (CELL_SIZE + WALL_SIZE) + WALL_SIZE;

            // Cell interior
            push_rect(cx, cy, CELL_SIZE, CELL_SIZE, cell_color);

            const auto cell_ptr = g.search(static_cast<int>(row * MAZE_COLS + col));
            if (!cell_ptr)
                continue;

            // Open east passage when this cell is linked to its eastern neighbour
            if (const auto east = g.get_east(cell_ptr); east && cell_ptr->is_linked(east))
                push_rect(cx + CELL_SIZE, cy, WALL_SIZE, CELL_SIZE, cell_color);

            // Open south passage when this cell is linked to its southern neighbour
            if (const auto south = g.get_south(cell_ptr); south && cell_ptr->is_linked(south))
                push_rect(cx, cy + CELL_SIZE, CELL_SIZE, WALL_SIZE, cell_color);
        }
    }

    // ------------------------------------------------------------------ //
    // Create the SFML window
    // ------------------------------------------------------------------ //
    const auto WIN_WIDTH  = static_cast<unsigned int>(MAZE_COLS * (CELL_SIZE + WALL_SIZE) + WALL_SIZE);
    const auto WIN_HEIGHT = static_cast<unsigned int>(MAZE_ROWS * (CELL_SIZE + WALL_SIZE) + WALL_SIZE);

    sf::RenderWindow window(
        sf::VideoMode({WIN_WIDTH, WIN_HEIGHT}),
        "AmazingSFML - MazeBuilder 2D"
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
                window.close();

            if (const auto* key = event->getIf<sf::Event::KeyPressed>())
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
        }

        window.clear(wall_color);
        window.draw(vertices);
        window.display();
    }

    return 0;
}
