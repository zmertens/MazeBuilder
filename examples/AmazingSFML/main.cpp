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
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    static const std::string APP_NAME = "AmazingSFML " + mazes::buildinfo::VERSION;
    static const std::filesystem::path MAZE_TEMP_IMAGE_PATH{std::filesystem::temp_directory_path() / "amazingsfml_maze.png"};
    static constexpr std::string_view ICON_FILEPATH{"icon.bmp"};

    static mazes::randomizer RNG{};

    constexpr unsigned int WINDOW_WIDTH = 800u;
    constexpr unsigned int WINDOW_HEIGHT = 600u;
    constexpr unsigned int MAZE_CELL_SIZE = 24u;
    constexpr unsigned int MAZE_ROWS = WINDOW_HEIGHT / MAZE_CELL_SIZE;
    constexpr unsigned int MAZE_COLS = WINDOW_WIDTH / MAZE_CELL_SIZE;
    constexpr float MAZE_WALL_SIZE = 4.f;
    constexpr float MAZE_PIXELS_PER_METER = MAZE_CELL_SIZE;
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

            const auto idx = static_cast<std::size_t>(row) * static_cast<std::size_t>(columns) + static_cast<std::size_t>(col);
            return &cells[idx];
        }
    };

    maze_topology build_topology_from_grid(const mazes::grid_interface &grid)
    {

        if (const auto &ops = grid.operations(); ops.is_valid_ops(&ops))
        {
            maze_topology out;
            auto [rows, columns, _] = ops.get_dimensions();
            out.rows = rows;
            out.columns = columns;
            out.cells.resize(static_cast<size_t>(rows) * static_cast<size_t>(columns));

            for (unsigned int row = 0; row < rows; ++row)
            {
                for (unsigned int col = 0; col < columns; ++col)
                {
                    const auto idx = static_cast<int>(row * columns + col);
                    if (const auto c = ops.search(idx))
                    {
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
            }

            return out;
        }
        return {};
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
            : current_wall_color(gen_random_color()), maze_sprite{maze_texture}, sfml_window(
                                                                                     sf::VideoMode(
                                                                                         {WINDOW_WIDTH, WINDOW_HEIGHT}),
                                                                                     APP_NAME, sf::Style::Titlebar | sf::Style::Close)

        {
            if (sf::Image icon = sf::Image{}; icon.loadFromFile(ICON_FILEPATH.data()))
            {
                sfml_window.setIcon(icon.getSize(), icon.getPixelsPtr());
            }

            sfml_window.setFramerateLimit(120u);
            sfml_window.setPosition({100, 100});
            load_font();
            init_help_text();
            rebuild_maze();
        }

        static sf::Color gen_random_color() noexcept
        {
            // Randomize hue across the full wheel so wall colors can be red,
            // orange, yellow, green, cyan, blue, purple, and in-between.
            const float hue = static_cast<float>(RNG.get_int(0, 359));
            const float saturation = static_cast<float>(RNG.get_int(55, 90)) / 100.0f;
            const float value = static_cast<float>(RNG.get_int(40, 78)) / 100.0f;

            const float chroma = value * saturation;
            const float h_prime = hue / 60.0f;
            const float x = chroma * (1.0f - std::abs(std::fmod(h_prime, 2.0f) - 1.0f));
            const float m = value - chroma;

            float r1 = 0.0f;
            float g1 = 0.0f;
            float b1 = 0.0f;

            if (h_prime < 1.0f)
            {
                r1 = chroma;
                g1 = x;
            }
            else if (h_prime < 2.0f)
            {
                r1 = x;
                g1 = chroma;
            }
            else if (h_prime < 3.0f)
            {
                g1 = chroma;
                b1 = x;
            }
            else if (h_prime < 4.0f)
            {
                g1 = x;
                b1 = chroma;
            }
            else if (h_prime < 5.0f)
            {
                r1 = x;
                b1 = chroma;
            }
            else
            {
                r1 = chroma;
                b1 = x;
            }

            const auto to_byte = [](const float channel) -> std::uint8_t
            {
                const float scaled = (channel * 255.0f);
                const int rounded = static_cast<int>(scaled + 0.5f);
                return static_cast<std::uint8_t>(std::clamp(rounded, 0, 255));
            };

            return sf::Color{
                to_byte(r1 + m),
                to_byte(g1 + m),
                to_byte(b1 + m)};
        }

        int run()
        {
            sf::Clock clock;
            float accumulator = 0.0f;
            constexpr float FIXED_DELTA = 1.0f / 120.0f;

            while (sfml_window.isOpen())
            {
                handle_events();

                accumulator += clock.restart().asSeconds();
                accumulator = std::min(accumulator, 0.25f);
                while (accumulator >= FIXED_DELTA)
                {
                    step_physics(FIXED_DELTA);
                    accumulator -= FIXED_DELTA;
                }

                sync_ball_drawables();

                sfml_window.clear(current_wall_color);
                if (has_maze_texture)
                {
                    sfml_window.draw(maze_sprite);
                }
                for (const auto &ball : physics_balls)
                {
                    sfml_window.draw(ball.drawable);
                }

                if (should_show_info)
                {
                    if (build_text.has_value())
                    {
                        sfml_window.draw(*build_text);
                    }
                    if (apply_timing_text.has_value())
                    {
                        sfml_window.draw(*apply_timing_text);
                    }
                    if (help_text.has_value())
                    {
                        sfml_window.draw(*help_text);
                    }
                }

                sfml_window.display();
            }

            return EXIT_SUCCESS;
        }

    private:
        const sf::Color current_wall_color;
        sf::RenderWindow sfml_window;
        std::optional<maze_topology> current_maze_struct;
        sf::Texture maze_texture;
        sf::Sprite maze_sprite;
        bool has_maze_texture{false};

        sf::Font sfml_font;
        bool should_show_info{true};
        std::optional<sf::Text> apply_timing_text;
        std::optional<sf::Text> build_text;
        std::optional<sf::Text> help_text;

        double last_apply_maze_in_ms{0.0};

        b2WorldId world_with_physics{b2_nullWorldId};
        std::optional<std::size_t> grabbed_ball_index;
        std::vector<b2BodyId> physics_wall_bodies;
        std::vector<dynamic_ball> physics_balls;

        void load_font()
        {
            const std::array<std::filesystem::path, 6> CANDIDATES{
                "C:/Windows/Fonts/consola.ttf",
                "C:/Windows/Fonts/arial.ttf",
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                "/usr/share/fonts/dejavu/DejaVuSans.ttf",
                "/System/Library/Fonts/SFNS.ttf",
                "/System/Library/Fonts/Supplemental/Arial.ttf"};

            // Get first candidate
            for (const auto &path : CANDIDATES)
            {
                if (std::filesystem::exists(path) && sfml_font.openFromFile(path))
                {
                    return;
                }
            }

            throw std::runtime_error("AmazingSFML cannot find a renderable font.");
        }

        void init_help_text()
        {
            build_text.emplace(sfml_font, "B: build new maze", 18u);
            build_text->setPosition({10.f, 10.f});
            build_text->setFillColor(sf::Color(245, 245, 235));
            build_text->setOutlineColor(sf::Color(15, 15, 15));
            build_text->setOutlineThickness(1.5f);

            help_text.emplace(sfml_font, "H: hide/show help", 18u);
            help_text->setPosition({10.f, 34.f});
            help_text->setFillColor(sf::Color(245, 245, 235));
            help_text->setOutlineColor(sf::Color(15, 15, 15));
            help_text->setOutlineThickness(1.5f);

            apply_timing_text.emplace(sfml_font, "Apply: -- ms", 18u);
            apply_timing_text->setFillColor(sf::Color(245, 245, 235));
            apply_timing_text->setOutlineColor(sf::Color(15, 15, 15));
            apply_timing_text->setOutlineThickness(1.2f);
            update_apply_timing_overlay();
        }

        void update_apply_timing_overlay()
        {
            if (!apply_timing_text)
            {
                return;
            }

            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << "Apply: " << last_apply_maze_in_ms << " ms";
            apply_timing_text->setString(oss.str());

            const auto bounds = apply_timing_text->getLocalBounds();
            const float x = 10.0f;
            const float y = static_cast<float>(sfml_window.getSize().y) - bounds.size.y - 12.0f;
            apply_timing_text->setPosition({x, y});
        }

        void create_world()
        {
            if (B2_IS_NON_NULL(world_with_physics))
            {
                b2DestroyWorld(world_with_physics);
            }

            b2WorldDef def = b2DefaultWorldDef();
            def.gravity = {0.0f, 9.8f};
            world_with_physics = b2CreateWorld(&def);
            physics_wall_bodies.clear();
            physics_balls.clear();
            grabbed_ball_index.reset();
        }

        static b2Vec2 px_to_m(const float x, const float y)
        {
            return {x / MAZE_PIXELS_PER_METER, y / MAZE_PIXELS_PER_METER};
        }

        void add_wall_body_from_rect(const float x, const float y, const float w, const float h)
        {
            b2BodyDef body_def = b2DefaultBodyDef();
            body_def.type = b2_staticBody;
            body_def.position = px_to_m(x + w * 0.5f, y + h * 0.5f);
            b2BodyId body = b2CreateBody(world_with_physics, &body_def);

            b2ShapeDef shape_def = b2DefaultShapeDef();
            const b2Polygon box = b2MakeBox((w * 0.5f) / MAZE_PIXELS_PER_METER, (h * 0.5f) / MAZE_PIXELS_PER_METER);
            b2CreatePolygonShape(body, &shape_def, &box);

            physics_wall_bodies.push_back(body);
        }

        void add_ball(const sf::Vector2f position)
        {
            b2BodyDef body_def = b2DefaultBodyDef();
            body_def.type = b2_dynamicBody;
            body_def.position = px_to_m(position.x, position.y);
            body_def.linearDamping = 0.08f;
            body_def.angularDamping = 0.10f;
            b2BodyId body = b2CreateBody(world_with_physics, &body_def);

            b2ShapeDef shape_def = b2DefaultShapeDef();
            shape_def.density = 1.0f;
            shape_def.material.friction = 0.3f;
            shape_def.material.restitution = 0.75f;
            const b2Circle circle = {{0.0f, 0.0f}, BALL_RADIUS_PIXELS / MAZE_PIXELS_PER_METER};
            b2CreateCircleShape(body, &shape_def, &circle);

            dynamic_ball ball{};
            ball.body = body;
            ball.drawable.setRadius(BALL_RADIUS_PIXELS);
            ball.drawable.setOrigin({BALL_RADIUS_PIXELS, BALL_RADIUS_PIXELS});
            ball.drawable.setFillColor(sf::Color(65, 122, 255));
            ball.drawable.setOutlineColor(sf::Color(18, 42, 92));
            ball.drawable.setOutlineThickness(1.5f);
            physics_balls.push_back(ball);
        }

        [[nodiscard]] std::optional<std::size_t> find_ball_at(const sf::Vector2f pos_pixels) const
        {
            const b2Vec2 target = px_to_m(pos_pixels.x, pos_pixels.y);
            float best_dist_sq = 1e9f;
            std::optional<std::size_t> best_index;

            for (std::size_t i = 0; i < physics_balls.size(); ++i)
            {
                const b2Vec2 p = b2Body_GetPosition(physics_balls[i].body);
                const float dx = target.x - p.x;
                const float dy = target.y - p.y;
                const float d2 = dx * dx + dy * dy;
                constexpr float max_pick_radius_m = (BALL_RADIUS_PIXELS * 2.2f) / MAZE_PIXELS_PER_METER;
                if (auto clamped_d2 = std::clamp(d2, 0.0f, max_pick_radius_m * max_pick_radius_m); clamped_d2 < best_dist_sq)
                {
                    best_dist_sq = clamped_d2;
                    best_index = i;
                }
            }

            return best_index;
        }

        void create_world_boundaries()
        {
            if (!current_maze_struct.has_value())
            {
                return;
            }

            const float world_w = static_cast<float>(current_maze_struct->columns) * (MAZE_CELL_SIZE + MAZE_WALL_SIZE);
            const float world_h = static_cast<float>(current_maze_struct->rows) * (MAZE_CELL_SIZE + MAZE_WALL_SIZE);

            add_wall_body_from_rect(-MAZE_WALL_SIZE, -MAZE_WALL_SIZE, world_w + MAZE_WALL_SIZE * 2.f, MAZE_WALL_SIZE);
            add_wall_body_from_rect(-MAZE_WALL_SIZE, world_h, world_w + MAZE_WALL_SIZE * 2.f, MAZE_WALL_SIZE);
            add_wall_body_from_rect(-MAZE_WALL_SIZE, 0.f, MAZE_WALL_SIZE, world_h);
            add_wall_body_from_rect(world_w, 0.f, MAZE_WALL_SIZE, world_h);
        }

        void build_geometry_and_physics()
        {
            if (!current_maze_struct.has_value())
            {
                return;
            }

            for (unsigned int row = 0u; row < current_maze_struct->rows; ++row)
            {
                for (unsigned int col = 0u; col < current_maze_struct->columns; ++col)
                {
                    const float cx = static_cast<float>(col) * (MAZE_CELL_SIZE + MAZE_WALL_SIZE) + MAZE_WALL_SIZE;
                    const float cy = static_cast<float>(row) * (MAZE_CELL_SIZE + MAZE_WALL_SIZE) + MAZE_WALL_SIZE;

                    const auto *cell = current_maze_struct->at(row, col);
                    if (!cell)
                    {
                        continue;
                    }

                    // Left/top boundaries come from the first row/column.
                    if (col == 0u && cell->west)
                    {
                        add_wall_body_from_rect(cx - MAZE_WALL_SIZE, cy, MAZE_WALL_SIZE, MAZE_CELL_SIZE);
                    }
                    if (row == 0u && cell->north)
                    {
                        add_wall_body_from_rect(cx, cy - MAZE_WALL_SIZE, MAZE_CELL_SIZE, MAZE_WALL_SIZE);
                    }

                    if (cell->east)
                    {
                        add_wall_body_from_rect(cx + MAZE_CELL_SIZE, cy, MAZE_WALL_SIZE, MAZE_CELL_SIZE);
                    }

                    if (cell->south)
                    {
                        add_wall_body_from_rect(cx, cy + MAZE_CELL_SIZE, MAZE_CELL_SIZE, MAZE_WALL_SIZE);
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
                      " --levels=1 --algo=" + std::string{mazes::to_sv_from_algo(RNG(0, 1) == 0 ? mazes::algo::DFS : mazes::algo::BINARY_TREE)} +
                      " --seed=" + std::to_string(RNG(1u, 4'200'000u)) +
                      " --output=" + MAZE_TEMP_IMAGE_PATH.string() + " --distances=[0:-1]";

            const auto apply_start = std::chrono::steady_clock::now();
            const auto result = app->apply(request);
            const auto apply_end = std::chrono::steady_clock::now();
            last_apply_maze_in_ms = std::chrono::duration<double, std::milli>(apply_end - apply_start).count();
            update_apply_timing_overlay();

            if (result.empty())
            {
                throw std::runtime_error("AmazingSFML failed to generate maze data.");
            }

            auto *generated_grid = app->get_last_grid();
            if (!generated_grid)
            {
                throw std::runtime_error("AmazingSFML failed to retrieve generated grid.");
            }

            if (!maze_texture.loadFromFile(MAZE_TEMP_IMAGE_PATH.string()))
            {
                throw std::runtime_error("AmazingSFML failed to load generated maze image.");
            }

            const auto image_size = maze_texture.getSize();
            if (image_size.x == 0u || image_size.y == 0u)
            {
                throw std::runtime_error("AmazingSFML generated an invalid maze image.");
            }

            const auto window_size = sfml_window.getSize();
            const float maze_scale_x = static_cast<float>(window_size.x) / static_cast<float>(image_size.x);
            const float maze_scale_y = static_cast<float>(window_size.y) / static_cast<float>(image_size.y);
            maze_sprite = sf::Sprite{maze_texture};
            maze_sprite.setScale({maze_scale_x, maze_scale_y});
            maze_sprite.setPosition({0.0f, 0.0f});
            has_maze_texture = true;

            // Keep topology in logical maze cells (rows/columns). Resizing to pixel
            // dimensions creates hundreds of thousands of cells and can OOM at launch.
            current_maze_struct = build_topology_from_grid(*generated_grid);

            build_geometry_and_physics();

            for (int i = 0; i < 24; ++i)
            {
                const float x = (MAZE_CELL_SIZE + MAZE_WALL_SIZE) * (1.0f + static_cast<float>(RNG(0, static_cast<int>(current_maze_struct->columns - 2u))));
                const float y = (MAZE_CELL_SIZE + MAZE_WALL_SIZE) * (0.6f + static_cast<float>(RNG(0, 4)) * 0.35f);
                add_ball({x, y});

                if (!physics_balls.empty())
                {
                    const b2Vec2 impulse{
                        static_cast<float>(RNG(-4, 4)) * 0.22f,
                        static_cast<float>(RNG(-1, 1)) * 0.15f};
                    b2Body_ApplyLinearImpulseToCenter(physics_balls.back().body, impulse, true);
                }
            }
        }

        void handle_events()
        {
            while (const auto event = sfml_window.pollEvent())
            {
                if (event->is<sf::Event::Closed>())
                {
                    sfml_window.close();
                }

                if (const auto *key = event->getIf<sf::Event::KeyPressed>())
                {
                    if (key->code == sf::Keyboard::Key::Escape)
                    {
                        sfml_window.close();
                    }
                    else if (key->code == sf::Keyboard::Key::B)
                    {
                        rebuild_maze();
                    }
                    else if (key->code == sf::Keyboard::Key::H)
                    {
                        should_show_info = !should_show_info;
                    }
                }

                if (const auto *mouse = event->getIf<sf::Event::MouseButtonPressed>())
                {
                    const sf::Vector2f pos{static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y)};
                    if (mouse->button == sf::Mouse::Button::Left)
                    {
                        if (const auto idx = find_ball_at(pos))
                        {
                            grabbed_ball_index = idx;
                            b2Body_SetAwake(physics_balls[*idx].body, true);
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
                            const b2Vec2 p = b2Body_GetPosition(physics_balls[*idx].body);
                            const b2Vec2 target = px_to_m(pos.x, pos.y);
                            const b2Vec2 impulse = {(target.x - p.x) * 3.0f, (target.y - p.y) * 3.0f};
                            b2Body_ApplyLinearImpulseToCenter(physics_balls[*idx].body, impulse, true);
                        }
                    }
                }

                if (event->is<sf::Event::MouseButtonReleased>())
                {
                    grabbed_ball_index.reset();
                }

                if (const auto *moved = event->getIf<sf::Event::MouseMoved>())
                {
                    if (grabbed_ball_index)
                    {
                        const sf::Vector2f pos{static_cast<float>(moved->position.x), static_cast<float>(moved->position.y)};
                        const b2Vec2 p = px_to_m(pos.x, pos.y);
                        b2Body_SetTransform(physics_balls[*grabbed_ball_index].body, p, b2Rot_identity);
                        b2Body_SetLinearVelocity(physics_balls[*grabbed_ball_index].body, {0.0f, 0.0f});
                    }
                }
            }
        }

        void step_physics(const float dt)
        {
            if (B2_IS_NON_NULL(world_with_physics))
            {
                b2World_Step(world_with_physics, dt, 4);
            }
        }

        void sync_ball_drawables()
        {
            for (auto &ball : physics_balls)
            {
                const b2Vec2 p = b2Body_GetPosition(ball.body);
                ball.drawable.setPosition({p.x * MAZE_PIXELS_PER_METER, p.y * MAZE_PIXELS_PER_METER});
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
