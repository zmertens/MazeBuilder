/// @file main.cpp
/// @brief AmazingSFML - gradient maze rendering with Box2D integration

#include <SFML/Graphics.hpp>

#include <box2d/box2d.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/singleton_base.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    constexpr unsigned int MAZE_ROWS = 20u;
    constexpr unsigned int MAZE_COLS = 20u;
    constexpr float CELL_SIZE = 24.f;
    constexpr float WALL_SIZE = 4.f;
    constexpr float PIXELS_PER_METER = CELL_SIZE;
    constexpr float BALL_RADIUS_PIXELS = 6.f;

    struct maze_cell_walls
    {
        bool north{true};
        bool south{true};
        bool east{true};
        bool west{true};
    };

    struct maze_topology
    {
        unsigned int rows{0};
        unsigned int columns{0};
        std::vector<maze_cell_walls> cells;

        [[nodiscard]] const maze_cell_walls *at(const unsigned int row, const unsigned int col) const noexcept
        {
            if (row >= rows || col >= columns)
            {
                return nullptr;
            }

            const auto idx = static_cast<size_t>(row) * static_cast<size_t>(columns) + static_cast<size_t>(col);
            return &cells[idx];
        }
    };

    maze_topology build_topology_from_grid(const mazes::grid_interface &grid, const unsigned int rows,
                                           const unsigned int columns)
    {
        maze_topology out;
        out.rows = rows;
        out.columns = columns;
        out.cells.resize(static_cast<size_t>(rows) * static_cast<size_t>(columns));

        const auto &ops = grid.operations();

        for (unsigned int row = 0; row < rows; ++row)
        {
            for (unsigned int col = 0; col < columns; ++col)
            {
                const auto idx = static_cast<int>(row * columns + col);
                const auto c = ops.search(idx);
                if (!c)
                {
                    continue;
                }

                const auto n = ops.get_north(c);
                const auto s = ops.get_south(c);
                const auto e = ops.get_east(c);
                const auto w = ops.get_west(c);

                maze_cell_walls cell_data;
                cell_data.north = !(n && c->is_linked(n));
                cell_data.south = !(s && c->is_linked(s));
                cell_data.east = !(e && c->is_linked(e));
                cell_data.west = !(w && c->is_linked(w));

                out.cells[static_cast<size_t>(row) * static_cast<size_t>(columns) + static_cast<size_t>(col)] =
                    cell_data;
            }
        }

        return out;
    }

    struct dynamic_ball
    {
        sf::CircleShape drawable{BALL_RADIUS_PIXELS};
        b2BodyId body{b2_nullBodyId};
    };

    class amazing_sfml_app
    {
    public:
        amazing_sfml_app()
            : m_wall_color(40, 40, 60)
            , m_maze_sprite{m_maze_texture}
            , m_window(
                                            sf::VideoMode(
                                                {static_cast<unsigned int>(MAZE_COLS * (CELL_SIZE + WALL_SIZE) + WALL_SIZE),
                                                 static_cast<unsigned int>(MAZE_ROWS * (CELL_SIZE + WALL_SIZE) + WALL_SIZE)}),
                                            "AmazingSFML + Box2D + MazeBuilder v" + mazes::buildinfo::Version)
                        
        {
            if (sf::Image icon = sf::Image{}; icon.loadFromFile("icon.bmp"))
            {
                m_window.setIcon(icon.getSize(), icon.getPixelsPtr());
            }

            m_window.setFramerateLimit(120u);
            m_rng.seed(m_rng(0u, 4'200'000u));
            load_font();
            init_help_text();
            rebuild_maze();
        }

        int run()
        {
            sf::Clock clock;
            float accumulator = 0.0f;
            constexpr float fixed_dt = 1.0f / 120.0f;

            while (m_window.isOpen())
            {
                handle_events();

                accumulator += clock.restart().asSeconds();
                accumulator = std::min(accumulator, 0.25f);
                while (accumulator >= fixed_dt)
                {
                    step_physics(fixed_dt);
                    accumulator -= fixed_dt;
                }

                sync_ball_drawables();

                m_window.clear(m_wall_color);
                if (m_has_maze_texture)
                {
                    m_window.draw(m_maze_sprite);
                }
                for (const auto &ball : m_balls)
                {
                    m_window.draw(ball.drawable);
                }
                if (m_show_help && m_help_text.has_value())
                {
                    m_window.draw(*m_help_text);
                }
                if (m_apply_timing_text.has_value())
                {
                    m_window.draw(*m_apply_timing_text);
                }
                m_window.display();
            }

            return EXIT_SUCCESS;
        }

    private:
        const sf::Color m_wall_color;
        sf::RenderWindow m_window;
        mazes::randomizer m_rng;
        std::optional<maze_topology> m_maze;
        sf::Texture m_maze_texture;
        sf::Sprite m_maze_sprite;
        bool m_has_maze_texture{false};
        const std::filesystem::path m_maze_image_path{std::filesystem::temp_directory_path() / "amazingsfml_maze.png"};

        sf::Font m_font;
        bool m_show_help{true};
        std::optional<sf::Text> m_help_text;
        std::optional<sf::Text> m_apply_timing_text;
        double m_last_apply_ms{0.0};
        std::optional<std::size_t> m_grabbed_ball;

        b2WorldId m_world{b2_nullWorldId};
        std::vector<b2BodyId> m_wall_bodies;
        std::vector<dynamic_ball> m_balls;

        void load_font()
        {
            const std::array<std::filesystem::path, 6> candidates{
                "C:/Windows/Fonts/consola.ttf",
                "C:/Windows/Fonts/arial.ttf",
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                "/usr/share/fonts/dejavu/DejaVuSans.ttf",
                "/System/Library/Fonts/SFNS.ttf",
                "/System/Library/Fonts/Supplemental/Arial.ttf"};

            for (const auto &path : candidates)
            {
                if (std::filesystem::exists(path) && m_font.openFromFile(path))
                {
                    return;
                }
            }

            throw std::runtime_error("AmazingSFML requires a readable TrueType font for the SFML::Text overlay");
        }

        void init_help_text()
        {
            m_help_text.emplace(m_font, "B: build new maze   H: hide/show help", 18u);
            m_help_text->setPosition({10.f, 10.f});
            m_help_text->setFillColor(sf::Color(245, 245, 235));
            m_help_text->setOutlineColor(sf::Color(15, 15, 15));
            m_help_text->setOutlineThickness(1.5f);

            m_apply_timing_text.emplace(m_font, "Apply: -- ms", 16u);
            m_apply_timing_text->setFillColor(sf::Color(245, 245, 235));
            m_apply_timing_text->setOutlineColor(sf::Color(15, 15, 15));
            m_apply_timing_text->setOutlineThickness(1.2f);
            update_apply_timing_overlay();
        }

        void update_apply_timing_overlay()
        {
            if (!m_apply_timing_text)
            {
                return;
            }

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << "Apply: " << m_last_apply_ms << " ms";
            m_apply_timing_text->setString(oss.str());

            const auto bounds = m_apply_timing_text->getLocalBounds();
            const float x = 10.0f;
            const float y = static_cast<float>(m_window.getSize().y) - bounds.size.y - 12.0f;
            m_apply_timing_text->setPosition({x, y});
        }

        void create_world()
        {
            if (B2_IS_NON_NULL(m_world))
            {
                b2DestroyWorld(m_world);
            }

            b2WorldDef def = b2DefaultWorldDef();
            def.gravity = {0.0f, 9.8f};
            m_world = b2CreateWorld(&def);
            m_wall_bodies.clear();
            m_balls.clear();
            m_grabbed_ball.reset();
        }

        static b2Vec2 px_to_m(const float x, const float y)
        {
            return {x / PIXELS_PER_METER, y / PIXELS_PER_METER};
        }

        void add_wall_body_from_rect(const float x, const float y, const float w, const float h)
        {
            b2BodyDef body_def = b2DefaultBodyDef();
            body_def.type = b2_staticBody;
            body_def.position = px_to_m(x + w * 0.5f, y + h * 0.5f);
            b2BodyId body = b2CreateBody(m_world, &body_def);

            b2ShapeDef shape_def = b2DefaultShapeDef();
            const b2Polygon box = b2MakeBox((w * 0.5f) / PIXELS_PER_METER, (h * 0.5f) / PIXELS_PER_METER);
            b2CreatePolygonShape(body, &shape_def, &box);

            m_wall_bodies.push_back(body);
        }

        void add_ball(const sf::Vector2f position)
        {
            b2BodyDef body_def = b2DefaultBodyDef();
            body_def.type = b2_dynamicBody;
            body_def.position = px_to_m(position.x, position.y);
            body_def.linearDamping = 0.08f;
            body_def.angularDamping = 0.10f;
            b2BodyId body = b2CreateBody(m_world, &body_def);

            b2ShapeDef shape_def = b2DefaultShapeDef();
            shape_def.density = 1.0f;
            shape_def.material.friction = 0.3f;
            shape_def.material.restitution = 0.75f;
            const b2Circle circle = {{0.0f, 0.0f}, BALL_RADIUS_PIXELS / PIXELS_PER_METER};
            b2CreateCircleShape(body, &shape_def, &circle);

            dynamic_ball ball{};
            ball.body = body;
            ball.drawable.setRadius(BALL_RADIUS_PIXELS);
            ball.drawable.setOrigin({BALL_RADIUS_PIXELS, BALL_RADIUS_PIXELS});
            ball.drawable.setFillColor(sf::Color(65, 122, 255));
            ball.drawable.setOutlineColor(sf::Color(18, 42, 92));
            ball.drawable.setOutlineThickness(1.5f);
            m_balls.push_back(ball);
        }

        [[nodiscard]] std::optional<std::size_t> find_ball_at(const sf::Vector2f pos_pixels) const
        {
            const b2Vec2 target = px_to_m(pos_pixels.x, pos_pixels.y);
            float best_dist_sq = 1e9f;
            std::optional<std::size_t> best_index;

            for (std::size_t i = 0; i < m_balls.size(); ++i)
            {
                const b2Vec2 p = b2Body_GetPosition(m_balls[i].body);
                const float dx = target.x - p.x;
                const float dy = target.y - p.y;
                const float d2 = dx * dx + dy * dy;
                constexpr float max_pick_radius_m = (BALL_RADIUS_PIXELS * 2.2f) / PIXELS_PER_METER;
                if (d2 <= max_pick_radius_m * max_pick_radius_m && d2 < best_dist_sq)
                {
                    best_dist_sq = d2;
                    best_index = i;
                }
            }

            return best_index;
        }

        void create_world_boundaries()
        {
            if (!m_maze.has_value())
            {
                return;
            }

            const float world_w = static_cast<float>(m_maze->columns) * (CELL_SIZE + WALL_SIZE);
            const float world_h = static_cast<float>(m_maze->rows) * (CELL_SIZE + WALL_SIZE);

            add_wall_body_from_rect(-WALL_SIZE, -WALL_SIZE, world_w + WALL_SIZE * 2.f, WALL_SIZE);
            add_wall_body_from_rect(-WALL_SIZE, world_h, world_w + WALL_SIZE * 2.f, WALL_SIZE);
            add_wall_body_from_rect(-WALL_SIZE, 0.f, WALL_SIZE, world_h);
            add_wall_body_from_rect(world_w, 0.f, WALL_SIZE, world_h);
        }

        void build_geometry_and_physics()
        {
            if (!m_maze.has_value())
            {
                return;
            }

            for (unsigned int row = 0u; row < m_maze->rows; ++row)
            {
                for (unsigned int col = 0u; col < m_maze->columns; ++col)
                {
                    const float cx = static_cast<float>(col) * (CELL_SIZE + WALL_SIZE) + WALL_SIZE;
                    const float cy = static_cast<float>(row) * (CELL_SIZE + WALL_SIZE) + WALL_SIZE;

                    const auto *cell = m_maze->at(row, col);
                    if (!cell)
                    {
                        continue;
                    }

                    // Left/top boundaries come from the first row/column.
                    if (col == 0u && cell->west)
                    {
                        add_wall_body_from_rect(cx - WALL_SIZE, cy, WALL_SIZE, CELL_SIZE);
                    }
                    if (row == 0u && cell->north)
                    {
                        add_wall_body_from_rect(cx, cy - WALL_SIZE, CELL_SIZE, WALL_SIZE);
                    }

                    if (cell->east)
                    {
                        add_wall_body_from_rect(cx + CELL_SIZE, cy, WALL_SIZE, CELL_SIZE);
                    }

                    if (cell->south)
                    {
                        add_wall_body_from_rect(cx, cy + CELL_SIZE, CELL_SIZE, WALL_SIZE);
                    }
                }
            }

            create_world_boundaries();
        }

        void rebuild_maze()
        {
            create_world();

            const auto app = mazes::singleton_base<mazes::runtime_app>::instance();
            if (!app)
            {
                throw std::runtime_error("AmazingSFML failed to initialize runtime app.");
            }

            std::string request;
            request.reserve(128);
            request = "--rows=" + std::to_string(MAZE_ROWS) +
                      " --columns=" + std::to_string(MAZE_COLS) +
                      " --levels=1 --algo=" + std::string{mazes::to_sv_from_algo(m_rng(0, 1) == 0 ? mazes::algo::DFS : mazes::algo::BINARY_TREE)} +
                      " --seed=" + std::to_string(m_rng(1u, 4'200'000u)) +
                      " --output=" + m_maze_image_path.string() + " --distances";

            const auto apply_start = std::chrono::steady_clock::now();
            const auto result = app->apply(request);
            const auto apply_end = std::chrono::steady_clock::now();
            m_last_apply_ms = std::chrono::duration<double, std::milli>(apply_end - apply_start).count();
            update_apply_timing_overlay();

            if (result.empty())
            {
                throw std::runtime_error("AmazingSFML failed to generate maze data.");
            }

            const auto *generated_grid = app->get_last_grid();
            if (!generated_grid)
            {
                throw std::runtime_error("AmazingSFML failed to retrieve generated grid.");
            }

            if (!m_maze_texture.loadFromFile(m_maze_image_path.string()))
            {
                throw std::runtime_error("AmazingSFML failed to load generated maze image.");
            }

            const auto image_size = m_maze_texture.getSize();
            if (image_size.x == 0u || image_size.y == 0u)
            {
                throw std::runtime_error("AmazingSFML generated an invalid maze image.");
            }

            const float expected_width = static_cast<float>(MAZE_COLS) * (CELL_SIZE + WALL_SIZE) + WALL_SIZE;
            const float expected_height = static_cast<float>(MAZE_ROWS) * (CELL_SIZE + WALL_SIZE) + WALL_SIZE;
            const float maze_scale_x = expected_width / static_cast<float>(image_size.x);
            const float maze_scale_y = expected_height / static_cast<float>(image_size.y);
            m_maze_sprite = sf::Sprite{m_maze_texture};
            m_maze_sprite.setScale({maze_scale_x, maze_scale_y});
            m_has_maze_texture = true;

            m_maze = build_topology_from_grid(*generated_grid, MAZE_ROWS, MAZE_COLS);

            build_geometry_and_physics();

            for (int i = 0; i < 24; ++i)
            {
                const float x = (CELL_SIZE + WALL_SIZE) * (1.0f + static_cast<float>(m_rng(0, static_cast<int>(m_maze->columns - 2u))));
                const float y = (CELL_SIZE + WALL_SIZE) * (0.6f + static_cast<float>(m_rng(0, 4)) * 0.35f);
                add_ball({x, y});

                if (!m_balls.empty())
                {
                    const b2Vec2 impulse{
                        static_cast<float>(m_rng(-4, 4)) * 0.22f,
                        static_cast<float>(m_rng(-1, 1)) * 0.15f};
                    b2Body_ApplyLinearImpulseToCenter(m_balls.back().body, impulse, true);
                }
            }
        }

        void handle_events()
        {
            while (const auto event = m_window.pollEvent())
            {
                if (event->is<sf::Event::Closed>())
                {
                    m_window.close();
                }

                if (const auto *key = event->getIf<sf::Event::KeyPressed>())
                {
                    if (key->code == sf::Keyboard::Key::Escape)
                    {
                        m_window.close();
                    }
                    else if (key->code == sf::Keyboard::Key::B)
                    {
                        rebuild_maze();
                    }
                    else if (key->code == sf::Keyboard::Key::H)
                    {
                        m_show_help = !m_show_help;
                    }
                }

                if (const auto *mouse = event->getIf<sf::Event::MouseButtonPressed>())
                {
                    const sf::Vector2f pos{static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y)};
                    if (mouse->button == sf::Mouse::Button::Left)
                    {
                        if (const auto idx = find_ball_at(pos))
                        {
                            m_grabbed_ball = idx;
                            b2Body_SetAwake(m_balls[*idx].body, true);
                        }
                        else
                        {
                            add_ball(pos);
                        }
                    }
                    else if (mouse->button == sf::Mouse::Button::Right)
                    {
                        if (const auto idx = find_ball_at(pos))
                        {
                            const b2Vec2 p = b2Body_GetPosition(m_balls[*idx].body);
                            const b2Vec2 target = px_to_m(pos.x, pos.y);
                            const b2Vec2 impulse = {(target.x - p.x) * 3.0f, (target.y - p.y) * 3.0f};
                            b2Body_ApplyLinearImpulseToCenter(m_balls[*idx].body, impulse, true);
                        }
                    }
                }

                if (event->is<sf::Event::MouseButtonReleased>())
                {
                    m_grabbed_ball.reset();
                }

                if (const auto *moved = event->getIf<sf::Event::MouseMoved>())
                {
                    if (m_grabbed_ball)
                    {
                        const sf::Vector2f pos{static_cast<float>(moved->position.x), static_cast<float>(moved->position.y)};
                        const b2Vec2 p = px_to_m(pos.x, pos.y);
                        b2Body_SetTransform(m_balls[*m_grabbed_ball].body, p, b2Rot_identity);
                        b2Body_SetLinearVelocity(m_balls[*m_grabbed_ball].body, {0.0f, 0.0f});
                    }
                }
            }
        }

        void step_physics(const float dt)
        {
            if (B2_IS_NON_NULL(m_world))
            {
                b2World_Step(m_world, dt, 4);
            }
        }

        void sync_ball_drawables()
        {
            for (auto &ball : m_balls)
            {
                const b2Vec2 p = b2Body_GetPosition(ball.body);
                ball.drawable.setPosition({p.x * PIXELS_PER_METER, p.y * PIXELS_PER_METER});
            }
        }
    };
}

int main()
{
    try
    {
        amazing_sfml_app app{};
        return app.run();
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << '\n';
        return EXIT_FAILURE;
    }
}
