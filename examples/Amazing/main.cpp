/// @file main.cpp
/// @brief Amazing - gradient maze rendering with Box2D integration

#include "game/animation.hpp"
#include "game/game_state.hpp"
#include "game/player_controller.hpp"
#include "game/resource_paths.hpp"
#include "game/ui_menu.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>

#include <box2d/box2d.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/bytes.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/topology.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/string_utils.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <queue>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

static const std::filesystem::path TEMP_IMAGE_PATH{ std::filesystem::temp_directory_path() / "amazing_maze.png" };
static const std::filesystem::path TEMP_TEXT_PATH{ std::filesystem::temp_directory_path() / "amazing_maze.txt" };
static const std::string APP_NAME = "Amazing - " + mazes::buildinfo::VERSION;

static mazes::randomizer RNG{};

constexpr unsigned int WINDOW_WIDTH = 800u;
constexpr unsigned int WINDOW_HEIGHT = 600u;

constexpr std::string_view NETWORK_HOST{ "localhost" };
constexpr unsigned short NETWORK_PORT = 8050u;
constexpr sf::Time NETWORK_CONNECT_TIMEOUT = sf::seconds(1.f);

// Must match maze_server::IMAGE_DELIMITER (examples/Http/maze_server.h).
constexpr std::string_view NETWORK_IMAGE_DELIMITER{ "\n--MAZE-IMAGE-BASE64--\n" };

// Must match pixels_create_state's fixed output geometry.
constexpr float TEXTURE_CELL_SIZE = 12.0f;
constexpr float TEXTURE_WALL_THICKNESS = 2.0f;

// GETs a maze from maze_server (see examples/Http) in a single request and returns the
// plain-text/binary-safe response body (metadata line + ASCII grid + base64 PNG), or
// nullopt on any failure.
std::optional<std::string> fetch_maze_over_http(const std::string_view host, const unsigned short port,
    const mazes::configurator& config)
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
    request << "GET /mazes?rows=" << config.rows() << "&columns=" << config.columns() << "&algo="
        << mazes::to_sv_from_algo(config.algo_id())
        << " HTTP/1.1\r\nHost: " << host << "\r\nConnection: close\r\n\r\n";
    const std::string request_str = request.str();
    if (socket.send(request_str.data(), request_str.size()) != sf::Socket::Status::Done)
    {
        return std::nullopt;
    }

    std::string response = "";
    std::array<char, 4096> buffer{};
    for (;;)
    {
        std::size_t received = 0u;
        if (socket.receive(buffer.data(), buffer.size(), received) != sf::Socket::Status::Done)
        {
            // Disconnected (server closes after response) or an error either way.
            break;
        }
        response.append(buffer.data(), received);
    }

    if (const auto header_end = response.find("\r\n\r\n"); header_end != std::string::npos)
    {
        return response.substr(response.size() < header_end + 4u ? response.size() : header_end + 4u);
    }
    return std::nullopt;
}

struct maze
{
    static std::uint32_t determine_cell_size(unsigned int window_width, unsigned int window_height)
    {
        return window_width - window_height > 0u ? window_height / 20u : window_width / 20u;
    }

    static float WALL_THICKNESS;
    static unsigned int CELL_SIZE;

    mazes::configurator config = mazes::configurator{}.algo_id(mazes::algo::BINARY_TREE).rows(CELL_SIZE).columns(CELL_SIZE).seed(RNG(0, mazes::configurator::DEFAULT_SEED_VALUE));
};

float maze::WALL_THICKNESS = static_cast<float>(maze::CELL_SIZE) / 8.f;
unsigned int maze::CELL_SIZE = maze::determine_cell_size(WINDOW_WIDTH, WINDOW_HEIGHT);

struct dynamic_ball
{
    static constexpr std::size_t NUM_BALLS = 24u;
    static float BALL_RADIUS_IN_PIXELS;

    sf::CircleShape drawable{ BALL_RADIUS_IN_PIXELS };

    b2BodyId body{ b2_nullBodyId };
};

float dynamic_ball::BALL_RADIUS_IN_PIXELS = static_cast<float>(maze::CELL_SIZE) / 15.f;

class amazing_sfml_app
{
public:
    static float PIXELS_PER_METER;

    amazing_sfml_app()
        : current_wall_color(gen_random_color()), maze_sprite{ maze_texture }, sfml_window(sf::VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }),
            APP_NAME,
            sf::Style::Resize | sf::Style::Titlebar | sf::Style::Close)

    {
        load_resource_paths();
        try_set_window_icon();
        load_menu_background();
        load_player_textures();

        sfml_window.setFramerateLimit(120u);
        sfml_window.setPosition({ 100, 100 });
        load_font();
        init_help_text();
        player_controller.reset();
        walk_animation.configure(0.14f, 2u);
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
        } else if (h_prime < 2.0f)
        {
            r1 = x;
            g1 = chroma;
        } else if (h_prime < 3.0f)
        {
            g1 = chroma;
            b1 = x;
        } else if (h_prime < 4.0f)
        {
            g1 = x;
            b1 = chroma;
        } else if (h_prime < 5.0f)
        {
            r1 = x;
            b1 = chroma;
        } else
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
            to_byte(b1 + m) };
    }

    void run() noexcept
    {
        sf::Clock clock;
        float accumulator = 0.0f;
        constexpr float FIXED_DELTA = 1.0f / 120.0f;

        while (sfml_window.isOpen())
        {
            const float frame_dt = clock.restart().asSeconds();
            handle_events();

            if (app_state.is_transition() && app_state.tick(frame_dt))
            {
                if (should_rebuild_after_transition)
                {
                    rebuild_maze();
                    should_rebuild_after_transition = false;
                }
                app_state.start_playing();
            }

            if (app_state.is_playing())
            {
                if (should_prefetch_level)
                {
                    prefetch_next_level();
                    should_prefetch_level = false;
                }

                accumulator += frame_dt;
                accumulator = std::min(accumulator, 0.25f);
                while (accumulator >= FIXED_DELTA)
                {
                    step_physics(FIXED_DELTA);
                    accumulator -= FIXED_DELTA;
                }

                sync_ball_drawables();
                update_player_sprite(frame_dt);
            } else
            {
                accumulator = 0.0f;
            }

            sfml_window.clear(current_wall_color);

            if (app_state.is_menu())
            {
                keyboard_menu.draw(sfml_window, sfml_font, has_menu_background_texture ? &menu_background_texture : nullptr);
            } else
            {
                if (has_maze_texture)
                {
                    sfml_window.draw(maze_sprite);
                } else
                {
                    for (const auto& wall_shape : maze_wall_shapes)
                    {
                        sfml_window.draw(wall_shape);
                    }
                }

                for (const auto& ball : physics_balls)
                {
                    sfml_window.draw(ball.drawable);
                }

                if (has_player_textures)
                {
                    sfml_window.draw(player_sprite);
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
            }

            sfml_window.display();
        }
    }

private:
    struct generated_level
    {
        sf::Image maze_image;
        mazes::topology topology;
    };

    const sf::Color current_wall_color;
    sf::RenderWindow sfml_window;
    std::optional<mazes::topology> current_maze_struct;
    sf::Texture maze_texture;
    sf::Sprite maze_sprite;
    bool has_maze_texture{ false };
    std::vector<sf::RectangleShape> maze_wall_shapes; // fallback rendering for network mazes (no texture)

    std::optional<mazes::configurator> current_maze;

    amazing::game::resource_paths resource_paths;
    amazing::game::game_state app_state;
    amazing::game::ui_menu keyboard_menu;
    amazing::game::player_controller player_controller;
    amazing::game::animation walk_animation;
    sf::Texture player_walk_a_texture;
    sf::Texture player_walk_b_texture;
    sf::Sprite player_sprite{ player_walk_a_texture };
    bool has_player_textures{ false };
    sf::Texture menu_background_texture;
    bool has_menu_background_texture{ false };
    bool should_rebuild_after_transition{ false };

    sf::Font sfml_font;
    bool should_show_info{ true };
    std::optional<sf::Text> apply_timing_text;
    std::optional<sf::Text> build_text;
    std::optional<sf::Text> help_text;
    std::optional<sf::Text> network_status_text;

    double how_long_last_apply_took{ 0.0 };

    b2WorldId world_with_physics{ b2_nullWorldId };
    b2BodyId player_body{ b2_nullBodyId };
    std::optional<std::size_t> grabbed_ball_index;
    std::optional<sf::Vector2f> grabbed_ball_start_position;
    std::vector<b2BodyId> physics_wall_bodies;
    std::vector<dynamic_ball> physics_balls;

    std::optional<generated_level> prefetched_level;
    bool should_prefetch_level{ false };
    std::vector<sf::Vector2f> active_solution_path;
    std::size_t active_solution_waypoint_index{ 0u };
    bool level_complete_pending{ false };

    // Physics geometry is built in its own "virtual" pixel space (maze::CELL_SIZE
    // based); these scale factors map that space onto the actual window so ball
    // rendering and mouse picking line up with the maze texture drawn on screen.
    float world_scale_x{ 1.0f };
    float world_scale_y{ 1.0f };

    float layout_cell_size{ TEXTURE_CELL_SIZE };
    float layout_wall_thickness{ TEXTURE_WALL_THICKNESS };

    [[nodiscard]] float layout_pitch() const noexcept
    {
        return layout_cell_size + layout_wall_thickness;
    }

    void update_screen_layout()
    {
        if (!current_maze_struct.has_value() || current_maze_struct->rows == 0u || current_maze_struct->columns == 0u)
        {
            world_scale_x = 1.0f;
            world_scale_y = 1.0f;
            return;
        }

        const auto window_size = sfml_window.getSize();
        const float world_w = static_cast<float>(current_maze_struct->columns) * layout_pitch() + layout_wall_thickness * 2.0f;
        const float world_h = static_cast<float>(current_maze_struct->rows) * layout_pitch() + layout_wall_thickness * 2.0f;
        world_scale_x = world_w > 0.0f ? static_cast<float>(window_size.x) / world_w : 1.0f;
        world_scale_y = world_h > 0.0f ? static_cast<float>(window_size.y) / world_h : 1.0f;

        if (has_maze_texture)
        {
            const auto image_size = maze_texture.getSize();
            maze_sprite.setScale({ static_cast<float>(window_size.x) / static_cast<float>(image_size.x),
                                  static_cast<float>(window_size.y) / static_cast<float>(image_size.y) });
            maze_sprite.setPosition({ 0.0f, 0.0f });
        }
    }

    [[nodiscard]] static std::optional<std::filesystem::path> find_existing_path(const std::filesystem::path& relative)
    {
        const std::array<std::filesystem::path, 3> candidates{
            relative,
            std::filesystem::current_path() / relative,
            std::filesystem::current_path().parent_path() / relative };

        for (const auto& candidate : candidates)
        {
            if (std::filesystem::exists(candidate))
            {
                return candidate;
            }
        }

        return std::nullopt;
    }

    void load_resource_paths()
    {
        if (const auto path = find_existing_path("resource_paths.json"); path.has_value())
        {
            if (resource_paths.load_from_file(path->string()))
            {
                return;
            }
        }

        throw std::runtime_error("Amazing failed to load resource_paths.json");
    }

    [[nodiscard]] std::optional<std::filesystem::path> resolve_resource_path(const std::string_view key) const
    {
        if (!resource_paths.contains(key))
        {
            return std::nullopt;
        }

        return find_existing_path(resource_paths.get(key));
    }

    void try_set_window_icon()
    {
        const auto icon_path = resolve_resource_path("icon");
        if (!icon_path.has_value())
        {
            return;
        }

        sf::Image icon;
        if (icon.loadFromFile(icon_path->string()))
        {
            sfml_window.setIcon(icon.getSize(), icon.getPixelsPtr());
        }
    }

    void load_menu_background()
    {
        const auto menu_path = resolve_resource_path("roguelike_menu");
        if (!menu_path.has_value())
        {
            has_menu_background_texture = false;
            return;
        }

        has_menu_background_texture = menu_background_texture.loadFromFile(menu_path->string());
    }

    void load_player_textures()
    {
        const auto walk_a_path = resolve_resource_path("character_beige_walk_a");
        const auto walk_b_path = resolve_resource_path("character_beige_walk_b");
        if (!walk_a_path.has_value() || !walk_b_path.has_value())
        {
            has_player_textures = false;
            return;
        }

        const bool loaded_a = player_walk_a_texture.loadFromFile(walk_a_path->string());
        const bool loaded_b = player_walk_b_texture.loadFromFile(walk_b_path->string());
        has_player_textures = loaded_a && loaded_b;
        if (!has_player_textures)
        {
            return;
        }

        player_sprite.setTexture(player_walk_a_texture, true);
        const auto size = player_walk_a_texture.getSize();
        player_sprite.setOrigin({ static_cast<float>(size.x) * 0.5f, static_cast<float>(size.y) * 0.5f });
        player_sprite.setScale({ 1.0f, 1.0f });
    }

    void update_player_sprite(const float dt_seconds)
    {
        if (!has_player_textures || !B2_IS_NON_NULL(player_body))
        {
            return;
        }

        if (player_controller.is_moving())
        {
            walk_animation.tick(dt_seconds);
        } else
        {
            walk_animation.reset();
        }

        const sf::Texture& active_texture = walk_animation.frame_index() == 0u ? player_walk_a_texture : player_walk_b_texture;
        player_sprite.setTexture(active_texture, true);

        const auto texture_size = active_texture.getSize();
        player_sprite.setOrigin({ static_cast<float>(texture_size.x) * 0.5f, static_cast<float>(texture_size.y) * 0.5f });

        const float target_diameter_x = player_radius_in_pixels() * 2.0f * world_scale_x;
        const float target_diameter_y = player_radius_in_pixels() * 2.0f * world_scale_y;
        const float scale_x = texture_size.x > 0u ? target_diameter_x / static_cast<float>(texture_size.x) : 1.0f;
        const float scale_y = texture_size.y > 0u ? target_diameter_y / static_cast<float>(texture_size.y) : 1.0f;
        player_sprite.setScale({ scale_x, scale_y });

        const b2Vec2 p = b2Body_GetPosition(player_body);
        player_sprite.setPosition(world_m_to_screen_px(p));
        player_sprite.setRotation(sf::degrees(player_controller.rotation_degrees()));
    }

    void load_font()
    {
        const std::array<std::filesystem::path, 6> CANDIDATES{
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/consola.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/dejavu/DejaVuSans.ttf",
            "/System/Library/Fonts/SFNS.ttf",
            "/System/Library/Fonts/Supplemental/Arial.ttf" };

        auto found = std::ranges::find_if(CANDIDATES, [this](const auto& path)
            { return std::filesystem::exists(path) && sfml_font.openFromFile(path); });

        if (found == CANDIDATES.cend())
        {
            throw std::runtime_error("Amazing cannot find a renderable font.");
        }
    }

    void init_help_text()
    {
        build_text.emplace(sfml_font, "B: rebuild maze queue\nEsc: open menu\n", 18u);
        build_text->setPosition({ 10.f, 10.f });
        build_text->setFillColor(sf::Color(245, 245, 235));
        build_text->setOutlineColor(sf::Color(15, 15, 15));
        build_text->setOutlineThickness(1.5f);

        help_text.emplace(sfml_font, "Left click ball: throw at character\nRight drag ball: fling\nH: hide/show help\nN: fetch maze via network\n", 18u);
        help_text->setPosition({ 10.f, 58.f });
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

        auto ms = fmt::format("{:.2f}", how_long_last_apply_took);
        apply_timing_text->setString("Apply took " + ms + " ms");

        const auto bounds = apply_timing_text->getLocalBounds();
        const float x = 10.0f;
        const float y = static_cast<float>(sfml_window.getSize().y) - bounds.size.y - 12.0f;
        apply_timing_text->setPosition({ x, y });
    }

    void set_network_status(const std::string& message)
    {
        if (!network_status_text)
        {
            network_status_text.emplace(sfml_font, message, 16u);
            network_status_text->setFillColor(sf::Color(235, 235, 120));
            network_status_text->setOutlineColor(sf::Color(15, 15, 15));
            network_status_text->setOutlineThickness(1.2f);
            network_status_text->setPosition({ 10.f, -58.f + static_cast<float>(sfml_window.getSize().y) });
        } else
        {
            network_status_text->setString(message);
        }
    }

    void throw_ball_at_character(const std::size_t ball_index, const float impulse_scale)
    {
        if (ball_index >= physics_balls.size() || !B2_IS_NON_NULL(player_body))
        {
            return;
        }

        const b2Vec2 ball_position = b2Body_GetPosition(physics_balls[ball_index].body);
        const b2Vec2 player_position = b2Body_GetPosition(player_body);
        const float dx = player_position.x - ball_position.x;
        const float dy = player_position.y - ball_position.y;
        const float len_sq = dx * dx + dy * dy;
        if (len_sq <= 1e-6f)
        {
            return;
        }

        const float inv_len = 1.0f / std::sqrt(len_sq);
        const b2Vec2 impulse{ dx * inv_len * impulse_scale, dy * inv_len * impulse_scale };
        b2Body_ApplyLinearImpulseToCenter(physics_balls[ball_index].body, impulse, true);
    }

    void rebuild_solution_path()
    {
        active_solution_path.clear();
        active_solution_waypoint_index = 0u;
        level_complete_pending = false;

        if (!current_maze_struct.has_value() || current_maze_struct->rows == 0u || current_maze_struct->columns == 0u)
        {
            return;
        }

        const unsigned int rows = current_maze_struct->rows;
        const unsigned int columns = current_maze_struct->columns;
        const auto to_index = [columns](const unsigned int row, const unsigned int col)
            {
                return static_cast<std::size_t>(row) * static_cast<std::size_t>(columns) + static_cast<std::size_t>(col);
            };

        const std::size_t total_cells = static_cast<std::size_t>(rows) * static_cast<std::size_t>(columns);
        std::vector<int> parents(total_cells, -1);
        std::queue<std::pair<unsigned int, unsigned int>> pending{};

        pending.push({ 0u, 0u });
        parents[to_index(0u, 0u)] = 0;

        while (!pending.empty())
        {
            const auto [row, col] = pending.front();
            pending.pop();

            if (row == rows - 1u && col == columns - 1u)
            {
                break;
            }

            const auto* cell = current_maze_struct->at(row, col);
            if (!cell)
            {
                continue;
            }

            const auto try_visit = [&](const unsigned int next_row, const unsigned int next_col)
                {
                    const std::size_t next_index = to_index(next_row, next_col);
                    if (parents[next_index] != -1)
                    {
                        return;
                    }

                    parents[next_index] = static_cast<int>(to_index(row, col));
                    pending.push({ next_row, next_col });
                };

            if (row > 0u && !cell->north())
            {
                try_visit(row - 1u, col);
            }
            if (col + 1u < columns && !cell->east())
            {
                try_visit(row, col + 1u);
            }
            if (row + 1u < rows && !cell->south())
            {
                try_visit(row + 1u, col);
            }
            if (col > 0u && !cell->west())
            {
                try_visit(row, col - 1u);
            }
        }

        const std::size_t goal_index = to_index(rows - 1u, columns - 1u);
        if (parents[goal_index] == -1)
        {
            return;
        }

        std::deque<std::size_t> reverse_path_indices;
        std::size_t walk_index = goal_index;
        reverse_path_indices.push_front(walk_index);
        while (walk_index != 0u)
        {
            walk_index = static_cast<std::size_t>(parents[walk_index]);
            reverse_path_indices.push_front(walk_index);
        }

        active_solution_path.reserve(reverse_path_indices.size());
        for (const std::size_t index : reverse_path_indices)
        {
            const auto row = static_cast<unsigned int>(index / columns);
            const auto col = static_cast<unsigned int>(index % columns);
            const float x = layout_wall_thickness + static_cast<float>(col) * layout_pitch() + layout_cell_size * 0.5f;
            const float y = layout_wall_thickness + static_cast<float>(row) * layout_pitch() + layout_cell_size * 0.5f;
            active_solution_path.push_back({ x, y });
        }
    }

    void advance_to_prefetched_level()
    {
        if (!prefetched_level.has_value())
        {
            rebuild_maze();
            return;
        }

        generated_level next_level = std::move(*prefetched_level);
        prefetched_level.reset();

        create_world();
        current_maze_struct = std::move(next_level.topology);

        if (maze_texture.loadFromImage(next_level.maze_image))
        {
            maze_sprite = sf::Sprite{ maze_texture };
            has_maze_texture = true;
            maze_wall_shapes.clear();
            update_screen_layout();
        } else
        {
            has_maze_texture = false;
            maze_wall_shapes.clear();
            update_screen_layout();
            build_wall_shapes_from_topology();
        }

        build_geometry_and_physics();
        create_player_body();
        spawn_random_balls(dynamic_ball::NUM_BALLS);
        player_controller.reset();
        walk_animation.reset();
        rebuild_solution_path();
        update_player_sprite(0.0f);
        should_prefetch_level = true;
    }

    void update_auto_solver()
    {
        if (!B2_IS_NON_NULL(player_body))
        {
            return;
        }

        if (active_solution_path.empty())
        {
            player_controller.set_move_direction({ 0.0f, 0.0f });
            return;
        }

        const b2Vec2 player_position_m = b2Body_GetPosition(player_body);
        const sf::Vector2f player_position_px{
            player_position_m.x * amazing_sfml_app::PIXELS_PER_METER,
            player_position_m.y * amazing_sfml_app::PIXELS_PER_METER };

        constexpr float WAYPOINT_REACHED_DISTANCE = 2.4f;
        while (active_solution_waypoint_index < active_solution_path.size())
        {
            const sf::Vector2f target = active_solution_path[active_solution_waypoint_index];
            const float dx = target.x - player_position_px.x;
            const float dy = target.y - player_position_px.y;
            if ((dx * dx + dy * dy) > WAYPOINT_REACHED_DISTANCE * WAYPOINT_REACHED_DISTANCE)
            {
                break;
            }
            ++active_solution_waypoint_index;
        }

        if (active_solution_waypoint_index >= active_solution_path.size())
        {
            player_controller.set_move_direction({ 0.0f, 0.0f });
            if (!level_complete_pending)
            {
                level_complete_pending = true;
                advance_to_prefetched_level();
            }
            return;
        }

        const sf::Vector2f target = active_solution_path[active_solution_waypoint_index];
        player_controller.set_move_direction({ target.x - player_position_px.x, target.y - player_position_px.y });
    }

    [[nodiscard]] generated_level generate_level_assets(const mazes::algo selected_algo, const unsigned int selected_seed)
    {
        const auto app = mazes::singleton_base<mazes::runtime_app>::instance();
        if (!app)
        {
            throw std::runtime_error("Amazing failed to initialize runtime app.");
        }

        auto mz{ mazes::configurator{}.ensure_algo_id(selected_algo).rows(maze::CELL_SIZE).columns(maze::CELL_SIZE).seed(selected_seed) };
        const auto image_path = mazes::string_utils::replace_all(TEMP_IMAGE_PATH.string(), "\\", "/");
        const auto text_path = mazes::string_utils::replace_all(TEMP_TEXT_PATH.string(), "\\", "/");

        const std::string image_request = "-j`{\"rows\":" +
            std::to_string(mz.rows()) +
            ",\"columns\":" + std::to_string(mz.columns()) +
            ",\"levels\":1"
            ",\"algo\":\"" +
            std::string{ mazes::to_sv_from_algo(selected_algo) } +
            "\",\"seed\":" + std::to_string(selected_seed) +
            ",\"output\":\"" + image_path +
            "\",\"distances\":\"[0:-1]\"}`";

        const std::string text_request = "-j`{\"rows\":" +
            std::to_string(mz.rows()) +
            ",\"columns\":" + std::to_string(mz.columns()) +
            ",\"levels\":1"
            ",\"algo\":\"" +
            std::string{ mazes::to_sv_from_algo(selected_algo) } +
            "\",\"seed\":" + std::to_string(selected_seed) +
            ",\"output\":\"" + text_path +
            "\",\"distances\":\"[0:-1]\"}`";

        const auto apply_start = std::chrono::steady_clock::now();
        const std::string image_result{ app->apply(image_request) };
        const std::string text_result{ app->apply(text_request) };
        const auto apply_end = std::chrono::steady_clock::now();
        how_long_last_apply_took = std::chrono::duration<double, std::milli>(apply_end - apply_start).count();

        fmt::print("Amazing: Requesting maze generation with: {}\n", image_request);
        fmt::print("Amazing: Requesting topology generation with: {}\n", text_request);
        fmt::print("Maze generation took {:.4f} ms\n", how_long_last_apply_took);

        if (image_result.empty() || text_result.empty())
        {
            throw std::runtime_error("Amazing failed to regenerate maze resources.");
        }

        std::ifstream text_file{ text_path, std::ios::binary };
        if (!text_file.is_open())
        {
            throw std::runtime_error("Amazing failed to open generated maze text file.");
        }

        std::ostringstream text_stream;
        text_stream << text_file.rdbuf();
        const std::string generated_grid = text_stream.str();
        if (generated_grid.empty())
        {
            throw std::runtime_error("Amazing failed to retrieve generated grid.");
        }

        sf::Image image;
        if (!image.loadFromFile(image_path))
        {
            throw std::runtime_error("Amazing failed to load generated maze image.");
        }

        const auto image_size = image.getSize();
        if (image_size.x == 0u || image_size.y == 0u)
        {
            throw std::runtime_error("Amazing generated an invalid maze image.");
        }

        mazes::topology topology = mazes::topology::parse(generated_grid);
        if (topology.rows == 0u || topology.columns < 2u)
        {
            throw std::runtime_error("Amazing parsed an invalid topology from generated grid text.");
        }

        return generated_level{ std::move(image), std::move(topology) };
    }

    void prefetch_next_level()
    {
        if (prefetched_level.has_value())
        {
            return;
        }

        const mazes::algo selected_algo = (RNG(0, 1) == 0) ? mazes::algo::DFS : mazes::algo::BINARY_TREE;
        const unsigned int selected_seed = RNG(1u, 4'200'000u);
        prefetched_level = generate_level_assets(selected_algo, selected_seed);
    }

    void create_world()
    {
        if (B2_IS_NON_NULL(world_with_physics))
        {
            b2DestroyWorld(world_with_physics);
        }

        b2WorldDef def = b2DefaultWorldDef();
        def.gravity = { 0.0f, 0.0f };
        world_with_physics = b2CreateWorld(&def);
        player_body = b2_nullBodyId;
        physics_wall_bodies.clear();
        physics_balls.clear();
        grabbed_ball_index.reset();
        grabbed_ball_start_position.reset();
    }

    static b2Vec2 px_to_m(const float x, const float y)
    {
        return { x / amazing_sfml_app::PIXELS_PER_METER, y / amazing_sfml_app::PIXELS_PER_METER };
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
        return { p.x * amazing_sfml_app::PIXELS_PER_METER * world_scale_x, p.y * amazing_sfml_app::PIXELS_PER_METER * world_scale_y };
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
        const b2Polygon box = b2MakeBox((w * 0.5f) / amazing_sfml_app::PIXELS_PER_METER, (h * 0.5f) / amazing_sfml_app::PIXELS_PER_METER);
        b2CreatePolygonShape(body, &shape_def, &box);

        physics_wall_bodies.push_back(body);
    }

    void create_player_body()
    {
        if (!current_maze_struct.has_value() || !B2_IS_NON_NULL(world_with_physics))
        {
            return;
        }

        const float spawn_x = layout_wall_thickness + layout_cell_size * 0.5f;
        const float spawn_y = layout_wall_thickness + layout_cell_size * 0.5f;

        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = b2_dynamicBody;
        body_def.position = px_to_m(spawn_x, spawn_y);
        body_def.linearDamping = 7.0f;
        body_def.angularDamping = 9.0f;
        player_body = b2CreateBody(world_with_physics, &body_def);

        b2ShapeDef shape_def = b2DefaultShapeDef();
        shape_def.density = 1.0f;
        shape_def.material.friction = 0.35f;
        shape_def.material.restitution = 0.2f;
        const b2Circle circle = { {0.0f, 0.0f}, player_radius_in_pixels() / amazing_sfml_app::PIXELS_PER_METER };
        b2CreateCircleShape(player_body, &shape_def, &circle);
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
        const b2Circle circle = { {0.0f, 0.0f}, dynamic_ball::BALL_RADIUS_IN_PIXELS / amazing_sfml_app::PIXELS_PER_METER };
        b2CreateCircleShape(body, &shape_def, &circle);

        dynamic_ball ball{};
        ball.body = body;
        ball.drawable.setRadius(dynamic_ball::BALL_RADIUS_IN_PIXELS);
        ball.drawable.setOrigin({ dynamic_ball::BALL_RADIUS_IN_PIXELS, dynamic_ball::BALL_RADIUS_IN_PIXELS });
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
            const auto max_pick_radius_m = (dynamic_ball::BALL_RADIUS_IN_PIXELS * 2.2f) / amazing_sfml_app::PIXELS_PER_METER;
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

        const float world_w = static_cast<float>(current_maze_struct->columns) * layout_pitch();
        const float world_h = static_cast<float>(current_maze_struct->rows) * layout_pitch();

        update_screen_layout();

        add_wall_body_from_rect(-layout_wall_thickness, -layout_wall_thickness, world_w + layout_wall_thickness * 2.f, layout_wall_thickness);
        add_wall_body_from_rect(-layout_wall_thickness, world_h, world_w + layout_wall_thickness * 2.f, layout_wall_thickness);
        add_wall_body_from_rect(-layout_wall_thickness, 0.f, layout_wall_thickness, world_h);
        add_wall_body_from_rect(world_w, 0.f, layout_wall_thickness, world_h);
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
                const float cx = static_cast<float>(col) * layout_pitch() + layout_wall_thickness;
                const float cy = static_cast<float>(row) * layout_pitch() + layout_wall_thickness;

                const auto* cw = current_maze_struct->at(row, col);
                if (!cw)
                {
                    continue;
                }

                // Left/top boundaries come from the first row/column.
                if (col == 0u && cw->west())
                {
                    add_wall_body_from_rect(cx - layout_wall_thickness, cy, layout_wall_thickness, layout_cell_size);
                }
                if (row == 0u && cw->north())
                {
                    add_wall_body_from_rect(cx, cy - layout_wall_thickness, layout_cell_size, layout_wall_thickness);
                }

                if (cw->east())
                {
                    add_wall_body_from_rect(cx + layout_cell_size, cy, layout_wall_thickness, layout_cell_size);
                }

                if (cw->south())
                {
                    add_wall_body_from_rect(cx, cy + layout_cell_size, layout_cell_size, layout_wall_thickness);
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
                sf::RectangleShape shape({ w * world_scale_x, h * world_scale_y });
                shape.setPosition({ x * world_scale_x, y * world_scale_y });
                shape.setFillColor(sf::Color(235, 235, 225));
                maze_wall_shapes.push_back(shape);
            };

        for (unsigned int row = 0u; row < current_maze_struct->rows; ++row)
        {
            for (unsigned int col = 0u; col < current_maze_struct->columns; ++col)
            {
                const float cx = static_cast<float>(col) * layout_pitch() + layout_wall_thickness;
                const float cy = static_cast<float>(row) * layout_pitch() + layout_wall_thickness;

                const auto* cell = current_maze_struct->at(row, col);
                if (!cell)
                {
                    continue;
                }

                if (col == 0u && cell->west())
                {
                    push_rect(cx - layout_wall_thickness, cy, layout_wall_thickness, layout_cell_size);
                }
                if (row == 0u && cell->north())
                {
                    push_rect(cx, cy - layout_wall_thickness, layout_cell_size, layout_wall_thickness);
                }
                if (cell->east())
                {
                    push_rect(cx + layout_cell_size, cy, layout_wall_thickness, layout_cell_size);
                }
                if (cell->south())
                {
                    push_rect(cx, cy + layout_cell_size, layout_cell_size, layout_wall_thickness);
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
            const float x = layout_pitch() * (1.0f + static_cast<float>(RNG(0, static_cast<int>(current_maze_struct->columns - 2u))));
            const float y = layout_pitch() * (0.6f + static_cast<float>(RNG(0, 4)) * 0.35f);
            // x/y above are virtual-space; convert to real window pixels for add_ball.
            add_ball({ x * world_scale_x, y * world_scale_y });

            if (!physics_balls.empty())
            {
                const b2Vec2 impulse{
                    static_cast<float>(RNG(-4, 4)) * 0.22f,
                    static_cast<float>(RNG(-1, 1)) * 0.15f };
                b2Body_ApplyLinearImpulseToCenter(physics_balls.back().body, impulse, true);
            }
        }
    }

    // Fetches a maze from maze_server (examples/Http) in a single request: the response
    // carries the ASCII grid plus a base64-encoded PNG, so no local apply() call is needed.
    void fetch_maze_from_network()
    {
        const std::string_view algo = mazes::ALGOS_LABELS_LOWERCASE.at(
            static_cast<std::size_t>(RNG(0, static_cast<int>(mazes::ALGOS_LABELS_LOWERCASE.size() - 1u))));

        const auto fetch_start = std::chrono::steady_clock::now();
        const auto body = fetch_maze_over_http(NETWORK_HOST, NETWORK_PORT, this->current_maze.value_or(mazes::configurator{}));
        const auto fetch_end = std::chrono::steady_clock::now();
        how_long_last_apply_took = std::chrono::duration<double, std::milli>(fetch_end - fetch_start).count();

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

        const std::string_view after_metadata = std::string_view{ *body }.substr(first_nl + 1u);
        const auto delimiter_pos = after_metadata.find(NETWORK_IMAGE_DELIMITER);
        const std::string_view grid_text = after_metadata.substr(0u, delimiter_pos);

        auto parsed_topology = mazes::topology::parse(grid_text);
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
                maze_sprite = sf::Sprite{ maze_texture };
                has_maze_texture = true;
                update_screen_layout();
            }
        }

        if (!has_maze_texture)
        {
            // Fall back to drawing wall rectangles when no usable image was received.
            build_wall_shapes_from_topology();
        }

        update_apply_timing_overlay();
        set_network_status(fmt::format("Network maze: {} ({}:{})", algo, NETWORK_HOST, NETWORK_PORT));

        create_player_body();
        spawn_random_balls(dynamic_ball::NUM_BALLS);
        player_controller.reset();
        walk_animation.reset();
        rebuild_solution_path();
        update_player_sprite(0.0f);
        should_prefetch_level = true;
    }

    void rebuild_maze()
    {
        const mazes::algo selected_algo = (RNG(0, 1) == 0) ? mazes::algo::DFS : mazes::algo::BINARY_TREE;
        const unsigned int selected_seed = RNG(1u, 4'200'000u);

        // Avoid stale file reads when generation fails.
        std::error_code ec;
        std::filesystem::remove(TEMP_IMAGE_PATH, ec);
        std::filesystem::remove(TEMP_TEXT_PATH, ec);

        generated_level level = generate_level_assets(selected_algo, selected_seed);

        create_world();
        current_maze_struct = std::move(level.topology);
        if (!maze_texture.loadFromImage(level.maze_image))
        {
            throw std::runtime_error("Amazing failed to load generated maze image.");
        }

        maze_sprite = sf::Sprite{ maze_texture };
        has_maze_texture = true;
        maze_wall_shapes.clear();
        network_status_text.reset();

        update_screen_layout();
        build_geometry_and_physics();
        create_player_body();
        spawn_random_balls(dynamic_ball::NUM_BALLS);
        player_controller.reset();
        walk_animation.reset();
        rebuild_solution_path();
        update_player_sprite(0.0f);
        update_apply_timing_overlay();

        prefetched_level.reset();
        prefetch_next_level();
        should_prefetch_level = false;
    }

    void handle_events()
    {
        while (const auto event = sfml_window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                sfml_window.close();
            }

            if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (app_state.is_menu())
                {
                    if (key->code == sf::Keyboard::Key::Escape)
                    {
                        sfml_window.close();
                    }

                    switch (keyboard_menu.on_key_pressed(key->code))
                    {
                    case amazing::game::menu_action::start:
                        app_state.start_playing();
                        break;
                    case amazing::game::menu_action::rebuild:
                        rebuild_maze();
                        app_state.start_playing();
                        break;
                    case amazing::game::menu_action::quit:
                        sfml_window.close();
                        break;
                    default:
                        break;
                    }
                } else
                {
                    if (key->code == sf::Keyboard::Key::Escape)
                    {
                        app_state.show_menu();
                    } else if (key->code == sf::Keyboard::Key::B)
                    {
                        app_state.begin_transition(0.30f);
                        should_rebuild_after_transition = true;
                    } else if (key->code == sf::Keyboard::Key::H)
                    {
                        should_show_info = !should_show_info;
                    } else if (key->code == sf::Keyboard::Key::N)
                    {
                        fetch_maze_from_network();
                    }
                }
            }

            if (!app_state.is_playing())
            {
                if (const auto* resized = event->getIf<sf::Event::Resized>())
                {
                    const float new_width = static_cast<float>(resized->size.x);
                    const float new_height = static_cast<float>(resized->size.y);
                    sfml_window.setView(sf::View{ {new_width * 0.5f, new_height * 0.5f}, {new_width, new_height} });
                    update_screen_layout();
                    update_apply_timing_overlay();
                    update_player_sprite(0.0f);
                }
                continue;
            }

            if (const auto* mouse = event->getIf<sf::Event::MouseButtonPressed>())
            {
                const sf::Vector2f pos{ static_cast<float>(mouse->position.x), static_cast<float>(mouse->position.y) };
                if (mouse->button == sf::Mouse::Button::Left)
                {
                    if (const auto idx = find_ball_at(pos))
                    {
                        throw_ball_at_character(*idx, 2.8f);
                    }
                } else if (mouse->button == sf::Mouse::Button::Right)
                {
                    if (const auto idx = find_ball_at(pos))
                    {
                        grabbed_ball_index = *idx;
                        grabbed_ball_start_position = pos;
                    }
                }
            }

            if (const auto* released = event->getIf<sf::Event::MouseButtonReleased>())
            {
                if (released->button == sf::Mouse::Button::Right && grabbed_ball_index.has_value())
                {
                    const sf::Vector2f release_position{ static_cast<float>(released->position.x), static_cast<float>(released->position.y) };
                    const sf::Vector2f start = grabbed_ball_start_position.value_or(release_position);
                    const sf::Vector2f delta = release_position - start;
                    const b2Vec2 delta_world = screen_px_to_world_m(delta.x, delta.y);
                    const float fling_strength = std::sqrt(delta_world.x * delta_world.x + delta_world.y * delta_world.y);

                    if (fling_strength > 0.02f)
                    {
                        constexpr float FLING_IMPULSE_SCALE = 3.4f;
                        const b2Vec2 impulse{ delta_world.x * FLING_IMPULSE_SCALE, delta_world.y * FLING_IMPULSE_SCALE };
                        b2Body_ApplyLinearImpulseToCenter(physics_balls[*grabbed_ball_index].body, impulse, true);
                    } else
                    {
                        throw_ball_at_character(*grabbed_ball_index, 3.3f);
                    }
                }

                grabbed_ball_index.reset();
                grabbed_ball_start_position.reset();
            }

            if (const auto* moved = event->getIf<sf::Event::MouseMoved>())
            {
                if (grabbed_ball_index)
                {
                    const sf::Vector2f pos{ static_cast<float>(moved->position.x), static_cast<float>(moved->position.y) };
                    const b2Vec2 p = screen_px_to_world_m(pos.x, pos.y);
                    b2Body_SetTransform(physics_balls[*grabbed_ball_index].body, p, b2Rot_identity);
                    b2Body_SetLinearVelocity(physics_balls[*grabbed_ball_index].body, { 0.0f, 0.0f });
                }
            }

            if (const auto* resized = event->getIf<sf::Event::Resized>())
            {
                const float new_width = static_cast<float>(resized->size.x);
                const float new_height = static_cast<float>(resized->size.y);
                sfml_window.setView(sf::View{ {new_width * 0.5f, new_height * 0.5f}, {new_width, new_height} });
                update_screen_layout();
                update_apply_timing_overlay();
                update_player_sprite(0.0f);
            }
        }
    }

    void step_physics(const float dt)
    {
        if (B2_IS_NON_NULL(world_with_physics))
        {
            if (B2_IS_NON_NULL(player_body))
            {
                update_auto_solver();
                const sf::Vector2f facing = player_controller.facing_direction();
                constexpr float PLAYER_SPEED_MPS = 3.25f;
                constexpr float VELOCITY_BLEND = 0.24f;
                const b2Vec2 current_velocity = b2Body_GetLinearVelocity(player_body);
                const float move_speed = player_controller.is_moving() ? PLAYER_SPEED_MPS : 0.0f;
                const b2Vec2 target_velocity{ facing.x * move_speed, facing.y * move_speed };
                const b2Vec2 blended_velocity{
                    current_velocity.x + (target_velocity.x - current_velocity.x) * VELOCITY_BLEND,
                    current_velocity.y + (target_velocity.y - current_velocity.y) * VELOCITY_BLEND };
                b2Body_SetLinearVelocity(player_body, blended_velocity);
                b2Body_SetAwake(player_body, true);
            }

            b2World_Step(world_with_physics, dt, 4);
        }
    }

    void sync_ball_drawables()
    {
        std::ranges::for_each(physics_balls, [this](dynamic_ball& ball)
            {
                const b2Vec2 p = b2Body_GetPosition(ball.body);
                ball.drawable.setPosition(world_m_to_screen_px(p));
                ball.drawable.setScale({ world_scale_x, world_scale_y }); });
    }

    [[nodiscard]] static float player_radius_in_pixels() noexcept
    {
        return dynamic_ball::BALL_RADIUS_IN_PIXELS * 2.22f;
    }
};

float amazing_sfml_app::PIXELS_PER_METER = maze::CELL_SIZE + maze::WALL_THICKNESS;

int main()
{
    try
    {
        amazing_sfml_app app{};
        app.run();
    } catch (const std::exception& ex)
    {
        fmt::print(stderr, "Unhandled exception: {}\n", ex.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
