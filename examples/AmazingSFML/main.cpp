/// @file main.cpp
/// @brief AmazingSFML - gradient maze rendering with Box2D integration

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include <box2d/box2d.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/bytes.h>
#include <MazeBuilder/cell.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/runtime_app.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
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
    static const std::filesystem::path MAZE_TEMP_TEXT_PATH{std::filesystem::temp_directory_path() / "amazingsfml_maze.txt"};
    static constexpr std::string_view ICON_FILEPATH{"icon.bmp"};

    static mazes::randomizer RNG{};

    constexpr unsigned int WINDOW_WIDTH = 800u;
    constexpr unsigned int WINDOW_HEIGHT = 600u;
    constexpr unsigned int MAZE_CELL_SIZE = 24u;
    constexpr unsigned int MAZE_ROWS = WINDOW_HEIGHT / MAZE_CELL_SIZE;
    constexpr unsigned int MAZE_COLS = WINDOW_WIDTH / MAZE_CELL_SIZE;
    constexpr float MAZE_WALL_SIZE = 4.f;
    constexpr float MAZE_PIXELS_PER_METER = MAZE_CELL_SIZE;
    constexpr float BALL_RADIUS_PIXELS = static_cast<float>(MAZE_CELL_SIZE) / 4.f;

    constexpr std::string_view NETWORK_HOST{"localhost"};
    constexpr unsigned short NETWORK_PORT = 8050u;
    constexpr sf::Time NETWORK_CONNECT_TIMEOUT = sf::seconds(1.f);
    constexpr std::array<std::string_view, 3> NETWORK_ALGOS{"binary_tree", "sidewinder", "dfs"};
    // Must match maze_server::IMAGE_DELIMITER (examples/Http/maze_server.h).
    constexpr std::string_view NETWORK_IMAGE_DELIMITER{"\n--MAZE-IMAGE-BASE64--\n"};

    // GETs a maze from maze_server (see examples/Http) in a single request and returns the
    // plain-text/binary-safe response body (metadata line + ASCII grid + base64 PNG), or
    // nullopt on any failure.
    std::optional<std::string> fetch_maze_over_http(const std::string_view host, const unsigned short port,
                                                     const unsigned int rows, const unsigned int columns,
                                                     const std::string_view algo)
    {
        const auto address = sf::IpAddress::resolve(host);
        if (!address.has_value())
        {
            return std::nullopt;
        }

        sf::TcpSocket socket;
        if (socket.connect(*address, port, NETWORK_CONNECT_TIMEOUT) != sf::Socket::Status::Done)
        {
            return std::nullopt;
        }

        std::ostringstream request;
        request << "GET /mazes?rows=" << rows << "&columns=" << columns << "&algo=" << algo
                << " HTTP/1.1\r\nHost: " << host << "\r\nConnection: close\r\n\r\n";
        const std::string request_str = request.str();
        if (socket.send(request_str.data(), request_str.size()) != sf::Socket::Status::Done)
        {
            return std::nullopt;
        }

        std::string response;
        std::array<char, 4096> buffer{};
        for (;;)
        {
            std::size_t received = 0u;
            if (socket.receive(buffer.data(), buffer.size(), received) != sf::Socket::Status::Done)
            {
                break; // Disconnected (server closes after response) or an error either way.
            }
            response.append(buffer.data(), received);
        }

        const auto header_end = response.find("\r\n\r\n");
        if (header_end == std::string::npos)
        {
            return std::nullopt;
        }

        return response.substr(response.size() < header_end + 4u ? response.size() : header_end + 4u);
    }

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

    maze_topology build_topology_from_grid(const std::string_view txt)
    {
        if (txt.empty())
        {
            return {};
        }

        std::vector<std::string_view> lines;
        lines.reserve(static_cast<std::size_t>(std::count(txt.begin(), txt.end(), '\n')) + 1u);

        std::size_t start = 0u;
        while (start <= txt.size())
        {
            const std::size_t end = txt.find('\n', start);
            std::string_view line = (end == std::string_view::npos)
                                        ? txt.substr(start)
                                        : txt.substr(start, end - start);

            if (!line.empty() && line.back() == '\r')
            {
                line.remove_suffix(1u);
            }

            if (!line.empty())
            {
                lines.push_back(line);
            }

            if (end == std::string_view::npos)
            {
                break;
            }
            start = end + 1u;
        }

        if (lines.empty() || lines.at(0).size() < 3u)
        {
            return {};
        }

        const std::string_view top_border = lines.front();
        std::vector<std::size_t> plus_positions;
        plus_positions.reserve(top_border.size());
        for (std::size_t i = 0u; i < top_border.size(); ++i)
        {
            if (top_border[i] == '+')
            {
                plus_positions.push_back(i);
            }
        }

        if (plus_positions.size() < 2u)
        {
            return {};
        }

        const auto rows_count = static_cast<unsigned int>((lines.size() - 1u) / 2u);
        const auto cols_count = static_cast<unsigned int>(plus_positions.size() - 1u);

        maze_topology out{};
        out.rows = rows_count;
        out.columns = cols_count;
        out.cells.resize(static_cast<std::size_t>(out.rows) * static_cast<std::size_t>(out.columns));

        auto has_horizontal_wall = [](const std::string_view border, const std::size_t from, const std::size_t to) -> bool
        {
            if (from >= border.size() || to > border.size() || from >= to)
            {
                return false;
            }

            for (std::size_t i = from; i < to; ++i)
            {
                if (border[i] == '-')
                {
                    return true;
                }
            }

            return false;
        };

        for (unsigned int row = 0u; row < out.rows; ++row)
        {
            const std::size_t top_line_index = 1u + static_cast<std::size_t>(row) * 2u;
            const std::size_t bottom_line_index = top_line_index + 1u;
            const std::size_t north_border_index = static_cast<std::size_t>(row) * 2u;

            if (bottom_line_index >= lines.size() || north_border_index >= lines.size())
            {
                break;
            }

            const std::string_view top_line = lines[top_line_index];
            const std::string_view bottom_line = lines[bottom_line_index];
            const std::string_view north_border = lines[north_border_index];

            for (unsigned int col = 0u; col < out.columns; ++col)
            {
                const std::size_t left = plus_positions[col];
                const std::size_t right = plus_positions[col + 1u];

                maze_cell_walls cell_data{};
                cell_data.north = has_horizontal_wall(north_border, left + 1u, right);
                cell_data.south = has_horizontal_wall(bottom_line, left + 1u, right);

                const std::size_t west_idx = left;
                const std::size_t east_idx = right;
                cell_data.west = (west_idx < top_line.size()) ? (top_line[west_idx] == '|') : true;
                cell_data.east = (east_idx < top_line.size()) ? (top_line[east_idx] == '|') : true;

                const auto idx = static_cast<std::size_t>(row) * static_cast<std::size_t>(out.columns) + static_cast<std::size_t>(col);
                out.cells[idx] = cell_data;
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
                else
                {
                    for (const auto &wall_shape : maze_wall_shapes)
                    {
                        sfml_window.draw(wall_shape);
                    }
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
                    if (network_status_text.has_value())
                    {
                        sfml_window.draw(*network_status_text);
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
        std::vector<sf::RectangleShape> maze_wall_shapes; // fallback rendering for network mazes (no texture)

        sf::Font sfml_font;
        bool should_show_info{true};
        std::optional<sf::Text> apply_timing_text;
        std::optional<sf::Text> build_text;
        std::optional<sf::Text> help_text;
        std::optional<sf::Text> network_status_text;

        double last_apply_maze_in_ms{0.0};

        b2WorldId world_with_physics{b2_nullWorldId};
        std::optional<std::size_t> grabbed_ball_index;
        std::vector<b2BodyId> physics_wall_bodies;
        std::vector<dynamic_ball> physics_balls;

        // Physics geometry is built in its own "virtual" pixel space (MAZE_CELL_SIZE
        // based); these scale factors map that space onto the actual window so ball
        // rendering and mouse picking line up with the maze texture drawn on screen.
        float world_scale_x{1.0f};
        float world_scale_y{1.0f};

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

            help_text.emplace(sfml_font, "H: hide/show help\nN: fetch maze via network", 18u);
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

        void set_network_status(const std::string &message)
        {
            if (!network_status_text)
            {
                network_status_text.emplace(sfml_font, message, 16u);
                network_status_text->setFillColor(sf::Color(235, 235, 120));
                network_status_text->setOutlineColor(sf::Color(15, 15, 15));
                network_status_text->setOutlineThickness(1.2f);
                network_status_text->setPosition({10.f, -58.f + static_cast<float>(sfml_window.getSize().y)});
            }
            else
            {
                network_status_text->setString(message);
            }
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

        // Converts a real window pixel (e.g. mouse position) into physics meters,
        // undoing the virtual-to-window stretch applied at render time.
        [[nodiscard]] b2Vec2 screen_px_to_world_m(const float x, const float y) const
        {
            return px_to_m(x / world_scale_x, y / world_scale_y);
        }

        // Converts a physics position (meters) into a real window pixel position.
        [[nodiscard]] sf::Vector2f world_m_to_screen_px(const b2Vec2 p) const
        {
            return {p.x * MAZE_PIXELS_PER_METER * world_scale_x, p.y * MAZE_PIXELS_PER_METER * world_scale_y};
        }

        void add_wall_body_from_rect(const float x, const float y, const float w, const float h)
        {
            if (w <= 0.0f || h <= 0.0f)
            {
                return;
            }

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
            body_def.position = screen_px_to_world_m(position.x, position.y);
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
            const b2Vec2 target = screen_px_to_world_m(pos_pixels.x, pos_pixels.y);
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

            if (current_maze_struct->rows == 0u || current_maze_struct->columns == 0u)
            {
                world_scale_x = 1.0f;
                world_scale_y = 1.0f;
                return;
            }

            const float world_w = static_cast<float>(current_maze_struct->columns) * (MAZE_CELL_SIZE + MAZE_WALL_SIZE);
            const float world_h = static_cast<float>(current_maze_struct->rows) * (MAZE_CELL_SIZE + MAZE_WALL_SIZE);

            world_scale_x = world_w > 0.0f ? static_cast<float>(WINDOW_WIDTH) / world_w : 1.0f;
            world_scale_y = world_h > 0.0f ? static_cast<float>(WINDOW_HEIGHT) / world_h : 1.0f;

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

        // Fallback renderer for network-fetched mazes, which have no PNG texture --
        // draws the same wall rectangles used for physics bodies, scaled to screen space.
        void build_wall_shapes_from_topology()
        {
            maze_wall_shapes.clear();
            if (!current_maze_struct.has_value())
            {
                return;
            }

            const auto push_rect = [this](const float x, const float y, const float w, const float h)
            {
                if (w <= 0.f || h <= 0.f)
                {
                    return;
                }
                sf::RectangleShape shape({w * world_scale_x, h * world_scale_y});
                shape.setPosition({x * world_scale_x, y * world_scale_y});
                shape.setFillColor(sf::Color(235, 235, 225));
                maze_wall_shapes.push_back(shape);
            };

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

                    if (col == 0u && cell->west)
                    {
                        push_rect(cx - MAZE_WALL_SIZE, cy, MAZE_WALL_SIZE, MAZE_CELL_SIZE);
                    }
                    if (row == 0u && cell->north)
                    {
                        push_rect(cx, cy - MAZE_WALL_SIZE, MAZE_CELL_SIZE, MAZE_WALL_SIZE);
                    }
                    if (cell->east)
                    {
                        push_rect(cx + MAZE_CELL_SIZE, cy, MAZE_WALL_SIZE, MAZE_CELL_SIZE);
                    }
                    if (cell->south)
                    {
                        push_rect(cx, cy + MAZE_CELL_SIZE, MAZE_CELL_SIZE, MAZE_WALL_SIZE);
                    }
                }
            }
        }

        // Spawns a batch of balls at randomized drop positions with a small random impulse.
        void spawn_random_balls(const int count)
        {
            if (!current_maze_struct.has_value() || current_maze_struct->columns < 2u)
            {
                return;
            }

            for (int i = 0; i < count; ++i)
            {
                const float x = (MAZE_CELL_SIZE + MAZE_WALL_SIZE) * (1.0f + static_cast<float>(RNG(0, static_cast<int>(current_maze_struct->columns - 2u))));
                const float y = (MAZE_CELL_SIZE + MAZE_WALL_SIZE) * (0.6f + static_cast<float>(RNG(0, 4)) * 0.35f);
                // x/y above are virtual-space; convert to real window pixels for add_ball.
                add_ball({x * world_scale_x, y * world_scale_y});

                if (!physics_balls.empty())
                {
                    const b2Vec2 impulse{
                        static_cast<float>(RNG(-4, 4)) * 0.22f,
                        static_cast<float>(RNG(-1, 1)) * 0.15f};
                    b2Body_ApplyLinearImpulseToCenter(physics_balls.back().body, impulse, true);
                }
            }
        }

        // Fetches a maze from maze_server (examples/Http) in a single request: the response
        // carries the ASCII grid plus a base64-encoded PNG, so no local apply() call is needed.
        void fetch_maze_from_network()
        {
            const std::string_view algo = NETWORK_ALGOS[static_cast<std::size_t>(RNG(0, static_cast<int>(NETWORK_ALGOS.size() - 1u)))];

            const auto fetch_start = std::chrono::steady_clock::now();
            const auto body = fetch_maze_over_http(NETWORK_HOST, NETWORK_PORT, MAZE_ROWS, MAZE_COLS, algo);
            const auto fetch_end = std::chrono::steady_clock::now();
            last_apply_maze_in_ms = std::chrono::duration<double, std::milli>(fetch_end - fetch_start).count();

            if (!body.has_value() || body->empty())
            {
                set_network_status(fmt::format("Network fetch failed ({}:{})", NETWORK_HOST, NETWORK_PORT));
                return;
            }

            // First line is a metadata header; the delimiter separates the ASCII grid from
            // the base64-encoded PNG that follows it.
            const auto first_nl = body->find('\n');
            if (first_nl == std::string::npos)
            {
                set_network_status("Network fetch returned a malformed response.");
                return;
            }

            const std::string_view after_metadata = std::string_view{*body}.substr(first_nl + 1u);
            const auto delimiter_pos = after_metadata.find(NETWORK_IMAGE_DELIMITER);
            const std::string_view grid_text = after_metadata.substr(0u, delimiter_pos);

            auto parsed_topology = build_topology_from_grid(grid_text);
            if (parsed_topology.rows == 0u || parsed_topology.columns < 2u)
            {
                set_network_status("Network fetch returned an invalid maze.");
                return;
            }

            create_world();
            current_maze_struct = std::move(parsed_topology);
            build_geometry_and_physics();

            has_maze_texture = false;
            maze_wall_shapes.clear();
            if (delimiter_pos != std::string_view::npos)
            {
                const std::string_view image_base64 = after_metadata.substr(delimiter_pos + NETWORK_IMAGE_DELIMITER.size());
                const std::string image_bytes = mazes::bytes::decode(image_base64);
                if (!image_bytes.empty() &&
                    maze_texture.loadFromMemory(image_bytes.data(), image_bytes.size()))
                {
                    const auto image_size = maze_texture.getSize();
                    const auto window_size = sfml_window.getSize();
                    maze_sprite = sf::Sprite{maze_texture};
                    maze_sprite.setScale({static_cast<float>(window_size.x) / static_cast<float>(image_size.x),
                                          static_cast<float>(window_size.y) / static_cast<float>(image_size.y)});
                    maze_sprite.setPosition({0.0f, 0.0f});
                    has_maze_texture = true;
                }
            }

            if (!has_maze_texture)
            {
                // Fall back to drawing wall rectangles when no usable image was received.
                build_wall_shapes_from_topology();
            }

            update_apply_timing_overlay();
            set_network_status(fmt::format("Network maze: {} ({}:{})", algo, NETWORK_HOST, NETWORK_PORT));

            spawn_random_balls(24);
        }


        void rebuild_maze()
        {
            create_world();

            const auto app = mazes::singleton_base<mazes::runtime_app>::instance();
            if (!app)
            {
                throw std::runtime_error("AmazingSFML failed to initialize runtime app.");
            }

            const mazes::algo selected_algo = (RNG(0, 1) == 0) ? mazes::algo::DFS : mazes::algo::BINARY_TREE;
            const unsigned int selected_seed = RNG(1u, 4'200'000u);

            // Avoid stale file reads when generation fails.
            std::error_code ec;
            std::filesystem::remove(MAZE_TEMP_IMAGE_PATH, ec);
            std::filesystem::remove(MAZE_TEMP_TEXT_PATH, ec);

            std::string image_request;
            image_request.reserve(160);
            image_request = "--rows=" + std::to_string(MAZE_ROWS) +
                            " --columns=" + std::to_string(MAZE_COLS) +
                            " --levels=1" +
                            " --algo=" + std::string{mazes::to_sv_from_algo(selected_algo)} +
                            " --seed=" + std::to_string(selected_seed) +
                            " --output=" + MAZE_TEMP_IMAGE_PATH.string() +
                            " --distances=[0:-1]";

            std::string text_request;
            text_request.reserve(160);
            text_request = "--rows=" + std::to_string(MAZE_ROWS) +
                           " --columns=" + std::to_string(MAZE_COLS) +
                           " --levels=1" +
                           " --algo=" + std::string{mazes::to_sv_from_algo(selected_algo)} +
                           " --seed=" + std::to_string(selected_seed) +
                           " --output=" + MAZE_TEMP_TEXT_PATH.string() +
                           " --distances=[0:-1]";

            const auto apply_start = std::chrono::steady_clock::now();
            const auto image_result = app->apply(image_request);
            const auto text_result = app->apply(text_request);
            const auto apply_end = std::chrono::steady_clock::now();
            last_apply_maze_in_ms = std::chrono::duration<double, std::milli>(apply_end - apply_start).count();

            fmt::print("AmazingSFML: Requesting maze generation with: {}\n", image_request);
            fmt::print("AmazingSFML: Requesting topology generation with: {}\n", text_request);
            fmt::print("Maze generation took {:.4f} ms\n", last_apply_maze_in_ms);

            if (image_result.empty() || text_result.empty())
            {
                throw std::runtime_error("AmazingSFML failed to regenerate maze resources.");
            }

            update_apply_timing_overlay();

            std::ifstream text_file{MAZE_TEMP_TEXT_PATH, std::ios::binary};
            if (!text_file.is_open())
            {
                throw std::runtime_error("AmazingSFML failed to open generated maze text file.");
            }

            std::ostringstream text_stream;
            text_stream << text_file.rdbuf();
            const std::string generated_grid = text_stream.str();
            if (generated_grid.empty())
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
            maze_wall_shapes.clear();
            network_status_text.reset();

            // Keep topology in logical maze cells (rows/columns). Resizing to pixel
            // dimensions creates hundreds of thousands of cells and can OOM at launch.
            current_maze_struct = build_topology_from_grid(generated_grid);
            if (!current_maze_struct.has_value() || current_maze_struct->rows == 0u || current_maze_struct->columns < 2u)
            {
                throw std::runtime_error("AmazingSFML parsed an invalid topology from generated grid text.");
            }

            build_geometry_and_physics();

            spawn_random_balls(24);
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
                    else if (key->code == sf::Keyboard::Key::N)
                    {
                        fetch_maze_from_network();
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
                            const b2Vec2 target = screen_px_to_world_m(pos.x, pos.y);
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
                        const b2Vec2 p = screen_px_to_world_m(pos.x, pos.y);
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
                ball.drawable.setPosition(world_m_to_screen_px(p));
                ball.drawable.setScale({world_scale_x, world_scale_y});
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
