/// @file main.cpp
/// @brief AmazingSFML - gradient maze rendering with Box2D integration

#include <SFML/Graphics.hpp>

#include <box2d/box2d.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/distance_grid.h>
#include <MazeBuilder/distances.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/singleton_base.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
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
        int distance{-1};
    };

    struct maze_topology
    {
        unsigned int rows{0};
        unsigned int columns{0};
        bool has_distances{false};
        int max_distance{0};
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

    maze_topology parse_ascii_topology(const std::string &ascii)
    {
        std::vector<std::string> lines;
        lines.reserve(256);

        std::string line;
        std::string current;
        current.reserve(256);
        for (const char ch : ascii)
        {
            if (ch == '\n')
            {
                lines.push_back(current);
                current.clear();
            }
            else if (ch != '\r')
            {
                current.push_back(ch);
            }
        }
        if (!current.empty())
        {
            lines.push_back(current);
        }

        if (lines.size() < 3)
        {
            throw std::runtime_error("AmazingSFML: maze text is too small.");
        }

        size_t max_width = 0;
        for (const auto &ln : lines)
        {
            max_width = std::max(max_width, ln.size());
        }
        for (auto &ln : lines)
        {
            ln.resize(max_width, ' ');
        }

        const auto ascii_h = static_cast<unsigned int>(lines.size());

        std::vector<unsigned int> plus_positions;
        plus_positions.reserve(max_width / 2u + 2u);
        for (unsigned int x = 0; x < static_cast<unsigned int>(max_width); ++x)
        {
            if (lines.front()[x] == '+')
            {
                plus_positions.push_back(x);
            }
        }

        if (plus_positions.size() < 2u || (ascii_h - 1u) % 2u != 0u)
        {
            throw std::runtime_error("AmazingSFML: unsupported maze text layout.");
        }

        const unsigned int stride = plus_positions[1] - plus_positions[0];
        if (stride < 2u)
        {
            throw std::runtime_error("AmazingSFML: unsupported maze text layout.");
        }

        for (size_t i = 1; i < plus_positions.size(); ++i)
        {
            if (plus_positions[i] - plus_positions[i - 1] != stride)
            {
                throw std::runtime_error("AmazingSFML: unsupported maze text layout.");
            }
        }

        const unsigned int cell_inner_width = stride - 1u;

        const auto parse_base36 = [](const std::string &token) -> std::optional<int>
        {
            if (token.empty())
            {
                return std::nullopt;
            }

            int value = 0;
            for (const char raw_ch : token)
            {
                const unsigned char uch = static_cast<unsigned char>(raw_ch);
                const char ch = static_cast<char>(std::toupper(uch));

                int digit = -1;
                if (ch >= '0' && ch <= '9')
                {
                    digit = ch - '0';
                }
                else if (ch >= 'A' && ch <= 'Z')
                {
                    digit = 10 + (ch - 'A');
                }
                else
                {
                    return std::nullopt;
                }

                if (value > (std::numeric_limits<int>::max() - digit) / 36)
                {
                    return std::nullopt;
                }

                value = value * 36 + digit;
            }

            return value;
        };

        maze_topology out;
        out.rows = (ascii_h - 1u) / 2u;
        out.columns = static_cast<unsigned int>(plus_positions.size() - 1u);
        out.cells.resize(static_cast<size_t>(out.rows) * static_cast<size_t>(out.columns));

        for (unsigned int row = 0; row < out.rows; ++row)
        {
            for (unsigned int col = 0; col < out.columns; ++col)
            {
                const unsigned int x0 = col * stride;
                const unsigned int y_top = row * 2u;
                const unsigned int y_mid = row * 2u + 1u;
                const unsigned int y_bottom = row * 2u + 2u;

                bool north = false;
                bool south = false;
                for (unsigned int x = x0 + 1u; x <= x0 + cell_inner_width; ++x)
                {
                    north = north || (lines[y_top][x] == '-');
                    south = south || (lines[y_bottom][x] == '-');
                }

                const bool west = lines[y_mid][x0] == '|';
                const bool east = lines[y_mid][x0 + stride] == '|';

                int distance = -1;
                {
                    const std::string cell_text = lines[y_mid].substr(x0 + 1u, cell_inner_width);
                    if (const auto start = cell_text.find_first_not_of(' '); start != std::string::npos)
                    {
                        const auto end = cell_text.find_last_not_of(' ');
                        if (const auto parsed = parse_base36(cell_text.substr(start, end - start + 1u)); parsed.has_value())
                        {
                            distance = *parsed;
                            out.has_distances = true;
                            out.max_distance = std::max(out.max_distance, distance);
                        }
                    }
                }

                const auto idx = static_cast<size_t>(row) * static_cast<size_t>(out.columns) + static_cast<size_t>(col);
                out.cells[idx] = {north, south, east, west, distance};
            }
        }

        return out;
    }

    maze_topology build_topology_from_grid(const mazes::grid_interface &grid, const unsigned int rows,
                                           const unsigned int columns)
    {
        maze_topology out;
        out.rows = rows;
        out.columns = columns;
        out.cells.resize(static_cast<size_t>(rows) * static_cast<size_t>(columns));

        const auto &ops = grid.operations();

        const mazes::distance_grid *dist_grid = dynamic_cast<const mazes::distance_grid *>(&grid);
        std::shared_ptr<mazes::distances> distances;
        std::shared_ptr<mazes::distances> shortest_path;
        if (dist_grid)
        {
            distances = dist_grid->get_distances();
            if (distances)
            {
                const int total_cells = static_cast<int>(rows * columns);
                int start_index = 0;
                for (int i = 0; i < total_cells; ++i)
                {
                    if (distances->contains(i) && (*distances)[i] == 0)
                    {
                        start_index = i;
                        break;
                    }
                }

                const auto [goal_index, _max_dist] = distances->max();
                shortest_path = mazes::distances::path_to(const_cast<mazes::grid_interface *>(&grid),
                                                          start_index,
                                                          goal_index);
                out.has_distances = static_cast<bool>(shortest_path);
                if (shortest_path)
                {
                    const auto [_, path_max] = shortest_path->max();
                    out.max_distance = path_max;
                }
            }
        }

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

                if (shortest_path && shortest_path->contains(idx))
                {
                    cell_data.distance = (*shortest_path)[idx];
                }

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
            : m_wall_color(40, 40, 60), m_vertices(sf::PrimitiveType::Triangles), m_window(
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
                m_window.draw(m_vertices);
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
        sf::VertexArray m_vertices;
        sf::RenderWindow m_window;
        mazes::randomizer m_rng;
        std::optional<maze_topology> m_maze;

        sf::Font m_font;
        bool m_show_help{true};
        std::optional<sf::Text> m_help_text;
        std::optional<sf::Text> m_apply_timing_text;
        double m_last_apply_ms{0.0};
        std::optional<std::size_t> m_grabbed_ball;

        b2WorldId m_world{b2_nullWorldId};
        std::vector<b2BodyId> m_wall_bodies;
        std::vector<dynamic_ball> m_balls;

        static sf::Color color_from_uint32(const std::uint32_t packed)
        {
            return {
                static_cast<std::uint8_t>((packed >> 16) & 0xFF),
                static_cast<std::uint8_t>((packed >> 8) & 0xFF),
                static_cast<std::uint8_t>(packed & 0xFF)};
        }

        static std::uint8_t lerp_u8(const std::uint8_t a, const std::uint8_t b, const float t)
        {
            const float clamped = std::clamp(t, 0.0f, 1.0f);
            return static_cast<std::uint8_t>(static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * clamped);
        }

        void push_rect(const float x, const float y, const float w, const float h, const sf::Color color)
        {
            sf::Vertex top_left, top_right, bottom_left, bottom_right;
            top_left.position = {x, y};
            top_left.color = color;
            top_right.position = {x + w, y};
            top_right.color = color;
            bottom_left.position = {x, y + h};
            bottom_left.color = color;
            bottom_right.position = {x + w, y + h};
            bottom_right.color = color;

            m_vertices.append(top_left);
            m_vertices.append(top_right);
            m_vertices.append(bottom_left);
            m_vertices.append(top_right);
            m_vertices.append(bottom_right);
            m_vertices.append(bottom_left);
        }

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

            m_vertices.clear();

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

                    sf::Color floor_color;
                    if (m_maze->has_distances && cell->distance >= 0)
                    {
                        const float t = m_maze->max_distance > 0
                                            ? static_cast<float>(cell->distance) / static_cast<float>(m_maze->max_distance)
                                            : 0.0f;
                        floor_color = {
                            lerp_u8(45u, 255u, t),
                            lerp_u8(95u, 70u, t),
                            lerp_u8(225u, 50u, t)};
                    }
                    else
                    {
                        const auto color_mod = static_cast<std::uint8_t>((row * 11u + col * 17u) % 70u);
                        floor_color = {
                            static_cast<std::uint8_t>(170u + color_mod),
                            static_cast<std::uint8_t>(170u + color_mod / 2u),
                            190u};
                    }
                    push_rect(cx, cy, CELL_SIZE, CELL_SIZE, floor_color);

                    // Left/top boundaries come from the first row/column.
                    if (col == 0u && cell->west)
                    {
                        add_wall_body_from_rect(cx - WALL_SIZE, cy, WALL_SIZE, CELL_SIZE);
                    }
                    if (row == 0u && cell->north)
                    {
                        add_wall_body_from_rect(cx, cy - WALL_SIZE, CELL_SIZE, WALL_SIZE);
                    }

                    if (!cell->east)
                    {
                        push_rect(cx + CELL_SIZE, cy, WALL_SIZE, CELL_SIZE, floor_color);
                    }
                    else
                    {
                        add_wall_body_from_rect(cx + CELL_SIZE, cy, WALL_SIZE, CELL_SIZE);
                    }

                    if (!cell->south)
                    {
                        push_rect(cx, cy + CELL_SIZE, CELL_SIZE, WALL_SIZE, floor_color);
                    }
                    else
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
                      " --output=sfml --distances";

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
