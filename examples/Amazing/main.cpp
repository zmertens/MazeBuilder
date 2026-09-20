/// @file main.cpp
/// @brief Interactive maze rendering with optional gradient coloring and physics

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>

#include <box2d/box2d.h>

#include <MazeBuilder/algos.h>
#include <MazeBuilder/buildinfo.h>
#include <MazeBuilder/bytes.h>
#include <MazeBuilder/configurator.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/runtime_app.h>
#include <MazeBuilder/string_utils.h>

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <future>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "utils.h"

static const std::string APP_NAME = "Amazing - " + mazes::buildinfo::VERSION;

enum class AppState
{
    MENU,
    PLAYING,
    TRANSITION,
    TUTORIAL
};

static std::variant<AppState> CURRENT_APP_STATE{AppState::TUTORIAL};

constexpr bool IS_APP_RUNNING(AppState state = AppState::MENU)
{
    return std::holds_alternative<AppState>(CURRENT_APP_STATE) && std::get<AppState>(CURRENT_APP_STATE) == state;
}

static std::vector<decltype(CURRENT_APP_STATE)> APP_STATE_HISTORY{};

static void switch_app_state(const AppState next_state)
{
    APP_STATE_HISTORY.push_back(CURRENT_APP_STATE);
    CURRENT_APP_STATE = next_state;
}

constexpr bool IS_APP_IN_HISTORY(decltype(CURRENT_APP_STATE) state)
{
    return std::ranges::any_of(APP_STATE_HISTORY, [&](const auto &s)
                               { return s == state; });
}

constexpr std::string_view FILE_NAMING_CONVENTION{"amazing_mazes"};

namespace resource_keys
{
    constexpr std::string_view ICON{"icon"};
    constexpr std::string_view FONT{"font"};

    constexpr std::string_view BLUR_FRAG{"blur_frag"};
    constexpr std::string_view RADIAL_FOG_FRAG{"radial_fog_frag"};
    constexpr std::string_view BLOOM_THRESHOLD_FRAG{"bloom_threshold_frag"};
    constexpr std::string_view PIXELATE_FRAG{"pixelate_frag"};
    constexpr std::string_view BILLBOARD_FRAG{"billboard_frag"};
    constexpr std::string_view BILLBOARD_BONUS_FRAG{"billboard_bonus_frag"};
    constexpr std::string_view BILLBOARD_VERT{"billboard_vert"};
    constexpr std::string_view BILLBOARD_GEOM{"billboard_geom"};
    constexpr std::string_view WAVE_VERT{"wave_vert"};

    constexpr std::string_view CHARACTER_IDLE{"character_beige_idle"};
    constexpr std::string_view MENU_SPRITE{"roguelike_menu"};

    constexpr std::string_view INTERACTION_SFX{"interaction"};
    constexpr std::string_view SYNTH_THEME{"synth_theme"};
}

static const std::filesystem::path TEMP_IMAGE_PATH{std::filesystem::temp_directory_path() / (std::string(FILE_NAMING_CONVENTION) + ".png")};
static const std::filesystem::path TEMP_TEXT_PATH{std::filesystem::temp_directory_path() / (std::string(FILE_NAMING_CONVENTION) + ".txt")};
static const std::filesystem::path RESOURCE_PATH{std::filesystem::current_path() / (std::string(FILE_NAMING_CONVENTION) + ".json")};

static mazes::randomizer RNG{};

auto *LOADER_INST = &utils::async_loader::instance();

struct scene_props : public sf::RenderWindow
{
    static constexpr auto WINDOW_PROPS{sf::Style::Resize | sf::Style::Titlebar | sf::Style::Close};
    static const sf::Vector2u INIT_WINDOW_SIZE;

    unsigned int current_cell_size;
    float pixels_per_meter;
    unsigned int wall_thickness;
    sf::Shader gameplay_shader;
    bool gameplay_shader_loaded;
    sf::Shader transition_shader;
    bool transition_shader_loaded;
    sf::Texture gameplay_sprite_texture;
    sf::Sprite gameplay_sprite{gameplay_sprite_texture};
    bool gameplay_sprite_loaded;
    sf::SoundBuffer interaction_sfx_buffer;
    sf::Sound interaction_sfx{interaction_sfx_buffer};
    bool interaction_sfx_loaded;

    scene_props()
        : sf::RenderWindow{sf::VideoMode{INIT_WINDOW_SIZE, 32}, APP_NAME, WINDOW_PROPS}, current_cell_size{determine_cell_size(INIT_WINDOW_SIZE.x, INIT_WINDOW_SIZE.y)}, pixels_per_meter{3.25f}, wall_thickness{1u}, gameplay_shader_loaded{false}, transition_shader_loaded{false}, gameplay_sprite_loaded{false}, interaction_sfx_loaded{false}
    {
    }

    static std::uint32_t determine_cell_size(unsigned int window_width, unsigned int window_height)
    {
        return window_width - window_height > 0u ? window_height / 20u : window_width / 20u;
    }

private:
};

const sf::Vector2u scene_props::INIT_WINDOW_SIZE{800u, 600u};

struct dynamic_ball
{
    static constexpr std::size_t NUM_BALLS = 24u;
    static float ball_radius_px;

    sf::Text drawable;
    b2BodyId body{b2_nullBodyId};
    bool collected{false};
    int value{1};
    float lifetime_seconds{0.0f};

    dynamic_ball() = delete;
    explicit dynamic_ball(const sf::Font &font, const std::string &text = "", const unsigned int character_size = 30u)
        : drawable{font, text, character_size}
    {
    }

    [[nodiscard]] bool is_positive() const noexcept
    {
        return value > 0;
    }
};

float dynamic_ball::ball_radius_px = static_cast<float>(scene_props::determine_cell_size(scene_props::INIT_WINDOW_SIZE.x, scene_props::INIT_WINDOW_SIZE.y)) / 15.f;

class amazing_sfml_app
{
public:
    static void run_self_tests()
    {
        const auto tmp_dir = std::filesystem::temp_directory_path() / "mazebuilder_async_loader_test";
        std::filesystem::remove_all(tmp_dir);
        std::filesystem::create_directories(tmp_dir / "resources");

        const auto json_path = tmp_dir / "amazing_mazes.json";
        std::ofstream resource_file{json_path};
        resource_file << "{\n"
                      << "  \"icon\": \"resources/icon.bmp\",\n"
                      << "  \"font\": \"resources/font.ttf\"\n"
                      << "}\n";
        resource_file.close();

        std::ofstream icon_file{tmp_dir / "resources" / "icon.bmp"};
        icon_file << "icon";
        std::ofstream font_file{tmp_dir / "resources" / "font.ttf"};
        font_file << "font";

        const auto resources = utils::async_loader::load_resource_map(json_path);
        if (resources.count(std::string{resource_keys::ICON}) != 1u || resources.count(std::string{resource_keys::FONT}) != 1u)
        {
            throw std::runtime_error("Resource loader smoke test failed: missing keys");
        }

        const auto icon = utils::async_loader::resolve_resource_path(resource_keys::ICON, json_path);
        if (!icon || *icon != (tmp_dir / "resources" / "icon.bmp"))
        {
            throw std::runtime_error("Resource loader smoke test failed: incorrect icon path");
        }

        std::filesystem::remove_all(tmp_dir);
    }

    amazing_sfml_app()
        : scene{std::make_unique<scene_props>()}

    {
        load_resource_paths();
        try_set_window_icon();

        scene->setFramerateLimit(120u);
        scene->setPosition({100, 100});
        load_font();
        init_help_text();
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

    void run() noexcept
    {
        sf::Clock clock;
        float accumulator = 0.0f;
        constexpr float FIXED_DELTA = 1.0f / 120.0f;

        while (scene->isOpen())
        {
            const float frame_dt = clock.restart().asSeconds();
            handle_events();
            update_state_overlay();
            update_focus_hold(frame_dt);

            if (IS_APP_RUNNING(AppState::TRANSITION))
            {
                update_transition(frame_dt);
            }

            if (IS_APP_RUNNING(AppState::PLAYING))
            {
                ensure_playing_assets_ready();

                accumulator += frame_dt;
                accumulator = std::min(accumulator, 0.25f);
                while (accumulator >= FIXED_DELTA)
                {
                    step_physics(FIXED_DELTA);
                    accumulator -= FIXED_DELTA;
                }

                update_number_balls(FIXED_DELTA);
                sync_ball_drawables();
            }
            else
            {
                accumulator = 0.0f;
            }

            update_bonus_particles(frame_dt);
            ensure_scene_surface_ready();
            if (!scene_surface_ready)
            {
                scene->clear(sf::Color{0, 0, 0, 255});
                scene->display();
                continue;
            }

            scene_surface.clear(sf::Color{0, 0, 0, 255});

            if (IS_APP_RUNNING(AppState::TUTORIAL) || IS_APP_RUNNING(AppState::PLAYING))
            {
                draw_tutorial_scene(scene_surface, IS_APP_RUNNING(AppState::TUTORIAL));
            }
            else if (IS_APP_RUNNING(AppState::TRANSITION))
            {
                draw_tutorial_scene(scene_surface, false);
            }
            else if (IS_APP_RUNNING(AppState::MENU))
            {
            }
            else
            {
                draw_playing_sprite_preview(scene_surface);
            }

            if (IS_APP_RUNNING(AppState::PLAYING) || IS_APP_RUNNING(AppState::TRANSITION))
            {
                draw_bonus_balls(scene_surface);
                draw_bonus_particles(scene_surface);
            }

            if (should_show_info)
            {
                if (state_label_text.has_value())
                {
                    scene_surface.draw(*state_label_text);
                }
                if (IS_APP_RUNNING(AppState::PLAYING) && score_text.has_value())
                {
                    scene_surface.draw(*score_text);
                }
            }

            scene_surface.display();

            scene->clear(sf::Color{0, 0, 0, 255});
            draw_post_processed_scene();

            scene->display();
        }
    }

private:
    std::unique_ptr<scene_props> scene;
    sf::Texture screen_texture;
    sf::Sprite screen_sprite{screen_texture};
    std::map<std::string, std::filesystem::path> loaded_resources{};

    sf::Font sfml_font;
    bool should_show_info{true};
    std::optional<sf::Text> apply_timing_text;
    std::optional<sf::Text> build_text;
    std::optional<sf::Text> help_text;
    std::optional<sf::Text> network_status_text;
    std::optional<sf::Text> score_text;
    std::optional<sf::Text> state_label_text;
    std::optional<sf::Text> tutorial_prompt_text;
    std::vector<sf::RectangleShape> tutorial_cells;
    std::vector<std::size_t> tutorial_drag_path{};
    std::vector<std::size_t> tutorial_bad_cells{};
    std::vector<std::size_t> tutorial_prize_path{};
    bool tutorial_drag_active{false};
    std::size_t tutorial_rows{8u};
    std::size_t tutorial_cols{8u};
    std::size_t tutorial_start_cell{0u};
    std::size_t tutorial_goal_cell{1u};
    std::string tutorial_prompt{"Tutorial: start on the glowing tile and swipe to the green prize."};
    int playing_score{0};
    int playing_streak{0};
    float transition_elapsed_seconds{0.0f};
    static constexpr float TRANSITION_DURATION_SECONDS = 1.35f;

    double how_long_last_apply_took{0.0};

    b2WorldId world_with_physics{b2_nullWorldId};
    b2BodyId player_body{b2_nullBodyId};
    std::optional<std::size_t> grabbed_ball_index;
    std::optional<sf::Vector2f> grabbed_ball_start_position;
    std::vector<b2BodyId> physics_wall_bodies;
    std::vector<dynamic_ball> physics_balls;

    sf::RenderTexture scene_surface;
    bool scene_surface_ready{false};
    sf::RenderTexture bloom_surface;
    bool bloom_surface_ready{false};
    sf::Shader fog_shader;
    bool fog_shader_loaded{false};
    sf::Shader bloom_threshold_shader;
    bool bloom_threshold_shader_loaded{false};
    sf::Shader bloom_shader;
    bool bloom_shader_loaded{false};
    sf::Shader ball_billboard_shader;
    bool ball_billboard_shader_loaded{false};
    sf::VertexArray bonus_ball_points{sf::PrimitiveType::Points};

    bool focus_hold_active{false};
    float focus_hold_seconds{0.0f};

    struct bonus_particle
    {
        sf::CircleShape drawable;
        sf::Vector2f velocity;
        float life_seconds;
    };

    std::vector<bonus_particle> bonus_particles;

    // Physics geometry is built in its own "virtual" pixel space (maze::CELL_SIZE
    // based); these scale factors map that space onto the actual window so ball
    // rendering and mouse picking line up with the maze texture drawn on screen.
    float world_scale_x{1.0f};
    float world_scale_y{1.0f};

    float layout_cell_size{1.f};
    float layout_wall_thickness{1.f};

    [[nodiscard]] float layout_pitch() const noexcept
    {
        return layout_cell_size + layout_wall_thickness;
    }

    void update_screen_layout()
    {
    }

    void update_focus_hold(const float dt)
    {
        if (focus_hold_active)
        {
            focus_hold_seconds = std::min(1.5f, focus_hold_seconds + dt);
        }
        else
        {
            focus_hold_seconds = std::max(0.0f, focus_hold_seconds - dt * 1.8f);
        }
    }

    [[nodiscard]] float focus_hold_ratio() const noexcept
    {
        return std::clamp(focus_hold_seconds / 1.5f, 0.0f, 1.0f);
    }

    void ensure_scene_surface_ready()
    {
        const auto size = scene->getSize();
        if (size.x == 0u || size.y == 0u)
        {
            scene_surface_ready = false;
            bloom_surface_ready = false;
            return;
        }

        if (scene_surface_ready && bloom_surface_ready && scene_surface.getSize() == size && bloom_surface.getSize() == size)
        {
            return;
        }

        scene_surface_ready = scene_surface.resize(size);
        bloom_surface_ready = bloom_surface.resize(size);
    }

    void draw_post_processed_scene()
    {
        const sf::Texture &frame_texture = scene_surface.getTexture();
        sf::Sprite frame{frame_texture};
        frame.setPosition({0.0f, 0.0f});

        if (IS_APP_RUNNING(AppState::TRANSITION) && scene->transition_shader_loaded)
        {
            const float t = std::clamp(transition_elapsed_seconds / TRANSITION_DURATION_SECONDS, 0.0f, 1.0f);
            const float wave_phase = t * 6.2831853f;
            const float wave_amp = 2.0f + 18.0f * (1.0f - std::abs(t * 2.0f - 1.0f));
            const float pixel_threshold = 0.02f + 0.18f * (1.0f - t);

            scene->transition_shader.setUniform("wave_phase", wave_phase);
            scene->transition_shader.setUniform("wave_amplitude", sf::Glsl::Vec2{wave_amp, wave_amp * 0.66f});
            scene->transition_shader.setUniform("pixel_threshold", pixel_threshold);

            sf::RenderStates states;
            states.shader = &scene->transition_shader;
            scene->draw(frame, states);
            return;
        }

        const bool should_apply_fog = IS_APP_RUNNING(AppState::PLAYING) || IS_APP_RUNNING(AppState::TUTORIAL);
        if (should_apply_fog && fog_shader_loaded)
        {
            const float reveal = focus_hold_ratio();
            const auto size = scene->getSize();
            const sf::Vector2i mouse = sf::Mouse::getPosition(*scene);
            const float nx = std::clamp(static_cast<float>(mouse.x) / static_cast<float>(std::max(1u, size.x)), 0.0f, 1.0f);
            const float ny = std::clamp(static_cast<float>(mouse.y) / static_cast<float>(std::max(1u, size.y)), 0.0f, 1.0f);

            const float clear_radius = 0.08f + reveal * 0.30f;
            const float feather = 0.14f - reveal * 0.04f;
            const float blur_radius = 0.022f - reveal * 0.019f;
            const float fog_darkness = 0.44f - reveal * 0.24f;

            fog_shader.setUniform("focus_center", sf::Glsl::Vec2{nx, ny});
            fog_shader.setUniform("clear_radius", clear_radius);
            fog_shader.setUniform("feather", std::max(0.03f, feather));
            fog_shader.setUniform("blur_radius", std::max(0.0014f, blur_radius));
            fog_shader.setUniform("fog_darkness", std::clamp(fog_darkness, 0.0f, 0.8f));

            sf::RenderStates states;
            states.shader = &fog_shader;
            scene->draw(frame, states);
        }
        else
        {
            scene->draw(frame);
        }

        if (should_apply_fog && bloom_shader_loaded && bloom_threshold_shader_loaded && bloom_surface_ready)
        {
            const float reveal = focus_hold_ratio();
            const float bloom_radius = 0.0032f + (1.0f - reveal) * 0.0028f;
            const float threshold = 0.58f + reveal * 0.12f;

            bloom_surface.clear(sf::Color{0, 0, 0, 0});
            bloom_threshold_shader.setUniform("threshold", std::clamp(threshold, 0.0f, 1.0f));
            bloom_threshold_shader.setUniform("intensity", 1.35f);
            sf::RenderStates bright_states;
            bright_states.shader = &bloom_threshold_shader;
            bloom_surface.draw(frame, bright_states);
            bloom_surface.display();

            sf::Sprite bloom_sprite{bloom_surface.getTexture()};
            bloom_sprite.setPosition({0.0f, 0.0f});

            bloom_shader.setUniform("blur_radius", bloom_radius);

            sf::RenderStates glow_states;
            glow_states.shader = &bloom_shader;
            glow_states.blendMode = sf::BlendAdd;
            scene->draw(bloom_sprite, glow_states);
        }
    }

    void emit_bonus_particles(const sf::Vector2f origin)
    {
        constexpr std::size_t PARTICLE_COUNT = 18u;
        for (std::size_t i = 0; i < PARTICLE_COUNT; ++i)
        {
            const float angle = static_cast<float>(RNG.get_int(0, 359)) * 3.14159265f / 180.0f;
            const float speed = static_cast<float>(RNG.get_int(45, 155));
            bonus_particle p{
                sf::CircleShape{static_cast<float>(RNG.get_int(2, 5))},
                {std::cos(angle) * speed, std::sin(angle) * speed},
                static_cast<float>(RNG.get_int(35, 90)) / 100.0f};
            p.drawable.setOrigin({p.drawable.getRadius(), p.drawable.getRadius()});
            p.drawable.setPosition(origin);
            p.drawable.setFillColor(sf::Color(255, 219, 88, 210));
            p.drawable.setOutlineThickness(0.6f);
            p.drawable.setOutlineColor(sf::Color(255, 248, 196));
            bonus_particles.push_back(p);
        }
    }

    void update_bonus_particles(const float dt)
    {
        for (auto &particle : bonus_particles)
        {
            particle.life_seconds -= dt;
            particle.drawable.move(particle.velocity * dt);
            particle.velocity *= 0.94f;
            const auto fill = particle.drawable.getFillColor();
            const auto alpha = static_cast<std::uint8_t>(std::clamp(particle.life_seconds, 0.0f, 1.0f) * 220.0f);
            particle.drawable.setFillColor(sf::Color(fill.r, fill.g, fill.b, alpha));
        }

        bonus_particles.erase(std::remove_if(bonus_particles.begin(), bonus_particles.end(), [](const bonus_particle &particle)
                                             { return particle.life_seconds <= 0.0f; }),
                             bonus_particles.end());
    }

    void draw_bonus_particles(sf::RenderTarget &target)
    {
        for (const auto &particle : bonus_particles)
        {
            target.draw(particle.drawable);
        }
    }

    void draw_bonus_balls(sf::RenderTarget &target)
    {
        for (const auto &ball : physics_balls)
        {
            if (ball.collected && ball.lifetime_seconds <= 0.0f)
            {
                continue;
            }

            target.draw(ball.drawable);
        }
    }

    [[nodiscard]] static std::optional<std::filesystem::path> find_existing_path(const std::filesystem::path &relative)
    {
        const std::array<std::filesystem::path, 3> candidates{
            relative,
            std::filesystem::current_path() / relative,
            std::filesystem::current_path().parent_path() / relative};

        for (const auto &candidate : candidates)
        {
            if (std::filesystem::exists(candidate))
            {
                return candidate;
            }
        }

        return std::nullopt;
    }

    [[nodiscard]] std::optional<std::filesystem::path> resolve_first_existing_resource(const std::initializer_list<std::string_view> keys) const
    {
        for (const auto key : keys)
        {
            if (const auto path = resolve_resource_path(key); path.has_value() && std::filesystem::exists(*path))
            {
                return path;
            }
        }

        return std::nullopt;
    }

    void load_resource_paths()
    {
        if (LOADER_INST == nullptr)
        {
            throw std::runtime_error("Amazing async loader instance was null");
        }

        if (!(*LOADER_INST))
        {
            throw std::runtime_error("Amazing async loader singleton was null");
        }

        (*LOADER_INST)->set_resource_path(RESOURCE_PATH);

        if (!std::filesystem::exists(RESOURCE_PATH) || RESOURCE_PATH.extension() != ".json")
        {
            throw std::runtime_error("Amazing failed to load resource_paths.json");
        }

        (*LOADER_INST)->load([this](std::map<std::string, std::filesystem::path> &resource_map)
                     { loaded_resources = resource_map; });

        if (loaded_resources.empty())
        {
            loaded_resources = utils::async_loader::load_resource_map(RESOURCE_PATH);
        }

        if (loaded_resources.empty())
        {
            throw std::runtime_error("Amazing resource map loaded but was empty");
        }
    }

    void ensure_shader_loaded()
    {
        if (scene->gameplay_shader_loaded)
        {
            return;
        }

        const auto shader_path = resolve_first_existing_resource({resource_keys::BLUR_FRAG, resource_keys::PIXELATE_FRAG, resource_keys::BILLBOARD_FRAG});
        if (!shader_path.has_value())
        {
            return;
        }

        scene->gameplay_shader_loaded = scene->gameplay_shader.loadFromFile(shader_path->string(), sf::Shader::Type::Fragment);
    }

    void ensure_fog_shader_loaded()
    {
        if (fog_shader_loaded)
        {
            return;
        }

        const auto radial_fog_frag = resolve_resource_path(resource_keys::RADIAL_FOG_FRAG);
        if (!radial_fog_frag.has_value())
        {
            return;
        }

        fog_shader_loaded = fog_shader.loadFromFile(radial_fog_frag->string(), sf::Shader::Type::Fragment);
    }

    void ensure_bloom_threshold_shader_loaded()
    {
        if (bloom_threshold_shader_loaded)
        {
            return;
        }

        const auto bloom_threshold_frag = resolve_resource_path(resource_keys::BLOOM_THRESHOLD_FRAG);
        if (!bloom_threshold_frag.has_value())
        {
            return;
        }

        bloom_threshold_shader_loaded = bloom_threshold_shader.loadFromFile(bloom_threshold_frag->string(), sf::Shader::Type::Fragment);
    }

    void ensure_bloom_shader_loaded()
    {
        if (bloom_shader_loaded)
        {
            return;
        }

        const auto blur_frag = resolve_resource_path(resource_keys::BLUR_FRAG);
        if (!blur_frag.has_value())
        {
            return;
        }

        bloom_shader_loaded = bloom_shader.loadFromFile(blur_frag->string(), sf::Shader::Type::Fragment);
    }

    void ensure_transition_shader_loaded()
    {
        if (scene->transition_shader_loaded)
        {
            return;
        }

        const auto wave_vert = resolve_resource_path(resource_keys::WAVE_VERT);
        const auto pixelate_frag = resolve_resource_path(resource_keys::PIXELATE_FRAG);
        if (!wave_vert.has_value() || !pixelate_frag.has_value())
        {
            return;
        }

        scene->transition_shader_loaded = scene->transition_shader.loadFromFile(
            wave_vert->string(),
            pixelate_frag->string());
    }

    void ensure_ball_billboard_shader_loaded()
    {
        if (ball_billboard_shader_loaded || !sf::Shader::isGeometryAvailable())
        {
            return;
        }

        const auto billboard_vert = resolve_resource_path(resource_keys::BILLBOARD_VERT);
        const auto billboard_geom = resolve_resource_path(resource_keys::BILLBOARD_GEOM);
        const auto billboard_frag = resolve_first_existing_resource({resource_keys::BILLBOARD_BONUS_FRAG, resource_keys::BILLBOARD_FRAG});
        if (!billboard_vert.has_value() || !billboard_geom.has_value() || !billboard_frag.has_value())
        {
            return;
        }

        ball_billboard_shader_loaded = ball_billboard_shader.loadFromFile(
            billboard_vert->string(),
            billboard_geom->string(),
            billboard_frag->string());
    }

    void ensure_sprite_loaded()
    {
        if (scene->gameplay_sprite_loaded)
        {
            return;
        }

        const auto sprite_path = resolve_first_existing_resource({resource_keys::CHARACTER_IDLE, resource_keys::MENU_SPRITE});
        if (!sprite_path.has_value())
        {
            return;
        }

        if (!scene->gameplay_sprite_texture.loadFromFile(sprite_path->string()))
        {
            return;
        }

        scene->gameplay_sprite_loaded = true;
    }

    void ensure_interaction_sfx_loaded()
    {
        if (scene->interaction_sfx_loaded)
        {
            return;
        }

        const auto sfx_path = resolve_first_existing_resource({resource_keys::INTERACTION_SFX, resource_keys::SYNTH_THEME});
        if (!sfx_path.has_value())
        {
            return;
        }

        if (!scene->interaction_sfx_buffer.loadFromFile(sfx_path->string()))
        {
            return;
        }

        scene->interaction_sfx.setBuffer(scene->interaction_sfx_buffer);
        scene->interaction_sfx_loaded = true;
    }

    void ensure_playing_assets_ready()
    {
        // Keep startup quick: load heavy media the first time PLAYING needs it.
        ensure_shader_loaded();
        ensure_fog_shader_loaded();
        ensure_bloom_threshold_shader_loaded();
        ensure_bloom_shader_loaded();
        ensure_transition_shader_loaded();
        ensure_ball_billboard_shader_loaded();
        ensure_sprite_loaded();
        ensure_interaction_sfx_loaded();
    }

    void begin_transition_to_playing()
    {
        transition_elapsed_seconds = 0.0f;
        ensure_playing_assets_ready();
        switch_app_state(AppState::TRANSITION);
    }

    void update_transition(const float dt)
    {
        transition_elapsed_seconds += dt;
        if (transition_elapsed_seconds >= TRANSITION_DURATION_SECONDS)
        {
            transition_elapsed_seconds = 0.0f;
            switch_app_state(AppState::PLAYING);
        }
    }

    void draw_transition_scene()
    {
        if (!scene->gameplay_sprite_loaded)
        {
            draw_tutorial_scene();
            return;
        }

        const auto size = scene->getSize();
        const auto texture_size = scene->gameplay_sprite_texture.getSize();
        if (texture_size.x == 0u || texture_size.y == 0u)
        {
            draw_tutorial_scene();
            return;
        }

        const float sx = static_cast<float>(size.x) / static_cast<float>(texture_size.x);
        const float sy = static_cast<float>(size.y) / static_cast<float>(texture_size.y);
        scene->gameplay_sprite.setPosition({0.0f, 0.0f});
        scene->gameplay_sprite.setScale({sx, sy});

        if (!scene->transition_shader_loaded)
        {
            scene->draw(scene->gameplay_sprite);
            return;
        }

        const float t = std::clamp(transition_elapsed_seconds / TRANSITION_DURATION_SECONDS, 0.0f, 1.0f);
        const float wave_phase = t * 6.2831853f;
        const float wave_amp = 2.0f + 18.0f * (1.0f - std::abs(t * 2.0f - 1.0f));
        const float pixel_threshold = 0.02f + 0.18f * (1.0f - t);

        scene->transition_shader.setUniform("wave_phase", wave_phase);
        scene->transition_shader.setUniform("wave_amplitude", sf::Glsl::Vec2{wave_amp, wave_amp * 0.66f});
        scene->transition_shader.setUniform("pixel_threshold", pixel_threshold);

        sf::RenderStates states;
        states.shader = &scene->transition_shader;
        scene->draw(scene->gameplay_sprite, states);

        if (tutorial_prompt_text.has_value())
        {
            tutorial_prompt_text->setString("Transitioning... entering PLAYING");
            tutorial_prompt_text->setPosition({14.f, static_cast<float>(size.y) - 42.f});
            scene->draw(*tutorial_prompt_text);
        }
    }

    void draw_playing_sprite_preview(sf::RenderTarget &target)
    {
        if (!scene->gameplay_sprite_loaded)
        {
            return;
        }

        const auto texture_size = scene->gameplay_sprite_texture.getSize();
        if (texture_size.x == 0u || texture_size.y == 0u)
        {
            return;
        }

        constexpr float target_height = 120.0f;
        const float scale = target_height / static_cast<float>(texture_size.y);
        scene->gameplay_sprite.setScale({scale, scale});
        scene->gameplay_sprite.setPosition({20.0f, 120.0f});

        if (scene->gameplay_shader_loaded)
        {
            sf::RenderStates states;
            states.shader = &scene->gameplay_shader;
            target.draw(scene->gameplay_sprite, states);
            return;
        }

        target.draw(scene->gameplay_sprite);
    }

    [[nodiscard]] std::optional<std::filesystem::path> resolve_resource_path(const std::string_view key) const
    {
        const auto it = loaded_resources.find(std::string{key});
        if (it != loaded_resources.cend())
        {
            return it->second;
        }

        const auto fallback = utils::async_loader::resolve_resource_path(key, RESOURCE_PATH);
        if (fallback.has_value())
        {
            return fallback;
        }

        return std::nullopt;
    }

    void try_set_window_icon()
    {
        const auto icon_path = resolve_resource_path(resource_keys::ICON);
        if (!icon_path.has_value())
        {
            return;
        }

        sf::Image icon;
        if (icon.loadFromFile(icon_path->string()))
        {
            scene->setIcon(icon.getSize(), icon.getPixelsPtr());
        }
    }

    void load_font()
    {
        if (const auto font_path = resolve_resource_path(resource_keys::FONT); font_path.has_value() && sfml_font.openFromFile(font_path->string()))
        {
            return;
        }

        const std::array<std::filesystem::path, 6> CANDIDATES{
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/consola.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/dejavu/DejaVuSans.ttf",
            "/System/Library/Fonts/SFNS.ttf",
            "/System/Library/Fonts/Supplemental/Arial.ttf"};

        auto found = std::ranges::find_if(CANDIDATES, [this](const auto &path)
                                          { return std::filesystem::exists(path) && sfml_font.openFromFile(path); });

        if (found == CANDIDATES.cend())
        {
            throw std::runtime_error("Amazing cannot find a renderable font.");
        }
    }

    void init_help_text()
    {
        build_text.emplace(sfml_font, "B: rebuild maze queue\nEsc: open menu\n", 18u);
        build_text->setPosition({10.f, 10.f});
        build_text->setFillColor(sf::Color(245, 245, 235));
        build_text->setOutlineColor(sf::Color(15, 15, 15));
        build_text->setOutlineThickness(1.5f);

        help_text.emplace(sfml_font, "Hold Space: focus through fog\nLeft click bonus orb: collect + score\nH: hide/show help\nN: fetch maze via network\n", 18u);
        help_text->setPosition({10.f, 58.f});
        help_text->setFillColor(sf::Color(245, 245, 235));
        help_text->setOutlineColor(sf::Color(15, 15, 15));
        help_text->setOutlineThickness(1.5f);

        apply_timing_text.emplace(sfml_font, "Apply: -- ms", 18u);
        apply_timing_text->setFillColor(sf::Color(245, 245, 235));
        apply_timing_text->setOutlineColor(sf::Color(15, 15, 15));
        apply_timing_text->setOutlineThickness(1.2f);

        score_text.emplace(sfml_font, "Score: 0  Streak: 0", 18u);
        score_text->setFillColor(sf::Color(255, 214, 102));
        score_text->setOutlineColor(sf::Color(40, 26, 8));
        score_text->setOutlineThickness(1.2f);

        state_label_text.emplace(sfml_font, "State: TUTORIAL", 18u);
        state_label_text->setFillColor(sf::Color(245, 245, 235));
        state_label_text->setOutlineColor(sf::Color(15, 15, 15));
        state_label_text->setOutlineThickness(1.2f);

        tutorial_prompt_text.emplace(sfml_font, tutorial_prompt, 20u);
        tutorial_prompt_text->setPosition({12.f, static_cast<float>(scene->getSize().y) - 42.f});
        tutorial_prompt_text->setFillColor(sf::Color(230, 245, 255));
        tutorial_prompt_text->setOutlineColor(sf::Color(15, 15, 15));
        tutorial_prompt_text->setOutlineThickness(1.2f);

        update_apply_timing_overlay();
        update_score_overlay();
        update_state_overlay();
        reset_tutorial_scene();
    }

    void update_score_overlay()
    {
        if (!score_text)
        {
            return;
        }

        score_text->setString(fmt::format("Score: {}  Streak: {}", playing_score, playing_streak));
        const auto bounds = score_text->getLocalBounds();
        const float x = static_cast<float>(scene->getSize().x) - bounds.size.x - 16.0f;
        score_text->setPosition({x, 10.0f});
        update_state_overlay();
    }

    [[nodiscard]] std::vector<std::size_t> build_tutorial_prize_path() const
    {
        std::vector<std::size_t> path{tutorial_start_cell};
        std::vector<bool> visited(tutorial_rows * tutorial_cols, false);
        visited[tutorial_start_cell] = true;

        std::size_t current = tutorial_start_cell;
        while (current != tutorial_goal_cell)
        {
            const auto current_row = current / tutorial_cols;
            const auto current_col = current % tutorial_cols;
            const auto goal_row = tutorial_goal_cell / tutorial_cols;
            const auto goal_col = tutorial_goal_cell % tutorial_cols;

            std::vector<std::size_t> candidates{};
            const auto add_candidate = [&](const std::size_t next_index)
            {
                if (next_index >= tutorial_rows * tutorial_cols || visited[next_index])
                {
                    return;
                }

                const auto next_row = next_index / tutorial_cols;
                const auto next_col = next_index % tutorial_cols;
                const auto distance_score = std::abs(static_cast<int>(next_row) - static_cast<int>(goal_row)) +
                                            std::abs(static_cast<int>(next_col) - static_cast<int>(goal_col));
                candidates.push_back(next_index);
                std::ranges::sort(candidates, [&](const std::size_t lhs, const std::size_t rhs)
                                  {
                                      const auto lhs_row = lhs / tutorial_cols;
                                      const auto lhs_col = lhs % tutorial_cols;
                                      const auto rhs_row = rhs / tutorial_cols;
                                      const auto rhs_col = rhs % tutorial_cols;
                                      const auto lhs_score = std::abs(static_cast<int>(lhs_row) - static_cast<int>(goal_row)) +
                                                            std::abs(static_cast<int>(lhs_col) - static_cast<int>(goal_col));
                                      const auto rhs_score = std::abs(static_cast<int>(rhs_row) - static_cast<int>(goal_row)) +
                                                            std::abs(static_cast<int>(rhs_col) - static_cast<int>(goal_col));
                                      return lhs_score < rhs_score; });
            };

            if (current_row > 0)
                add_candidate(current - tutorial_cols);
            if (current_row + 1 < tutorial_rows)
                add_candidate(current + tutorial_cols);
            if (current_col > 0)
                add_candidate(current - 1);
            if (current_col + 1 < tutorial_cols)
                add_candidate(current + 1);

            if (candidates.empty())
            {
                const auto fallback = std::max<std::size_t>(1u, tutorial_rows * tutorial_cols / 2u);
                current = (current + fallback) % (tutorial_rows * tutorial_cols);
                if (visited[current])
                {
                    break;
                }
                visited[current] = true;
                path.push_back(current);
                continue;
            }

            current = candidates.front();
            visited[current] = true;
            path.push_back(current);
        }

        if (path.empty() || path.back() != tutorial_goal_cell)
        {
            return {tutorial_start_cell, tutorial_goal_cell};
        }

        return path;
    }

    void reset_tutorial_scene()
    {
        constexpr std::array<std::string_view, 4> algos_as_str{"dfs", "binary_tree", "sidewinder", "prims"};
        tutorial_rows = static_cast<std::size_t>(std::clamp<int>(8 + RNG.get_int(0, 4), 8, 14));
        tutorial_cols = static_cast<std::size_t>(std::clamp<int>(8 + RNG.get_int(0, 4), 8, 14));
        tutorial_start_cell = static_cast<std::size_t>(RNG.get_int(0, static_cast<int>(tutorial_rows * tutorial_cols - 1)));
        tutorial_goal_cell = static_cast<std::size_t>(std::max<int>(1, static_cast<int>(tutorial_rows * tutorial_cols - 1 - RNG.get_int(0, 4))));
        tutorial_drag_path.clear();
        tutorial_bad_cells.clear();
        tutorial_drag_active = false;
        tutorial_prize_path = build_tutorial_prize_path();

        const auto runtime_inst = mazes::runtime_app::instance();
        const std::string args = "-r " + std::to_string(tutorial_rows) +
                                 " -c " + std::to_string(tutorial_cols) +
                                 " -a " + std::string{algos_as_str.at(static_cast<std::size_t>(RNG.get_int(0, 3)))} +
                                 " --distances=[0:-1]";
        if (runtime_inst != nullptr)
        {
            const auto maze_text = runtime_inst->apply(args);
            (void)maze_text;
            mazes::global_async_logger().log(fmt::format("Tutorial scene generated with args: {}", args));
        }

        tutorial_prompt = "Tutorial: start on the glowing tile and follow the hidden prize path to the green prize.";
        if (tutorial_prompt_text.has_value())
        {
            tutorial_prompt_text->setString(tutorial_prompt);
        }

        build_geometry_and_physics();
    }

    [[nodiscard]] bool tutorial_cell_is_on_prize_path(const std::size_t index) const noexcept
    {
        return std::ranges::find(tutorial_prize_path, index) != tutorial_prize_path.end();
    }

    [[nodiscard]] std::optional<std::size_t> tutorial_cell_from_pixel(const sf::Vector2f pixel) const
    {
        const auto area_size = scene->getSize();
        const float margin_x = 28.0f;
        const float margin_y = 60.0f;
        const float cell_w = (static_cast<float>(area_size.x) - margin_x * 2.0f) / static_cast<float>(tutorial_cols);
        const float cell_h = (static_cast<float>(area_size.y) - margin_y * 2.0f) / static_cast<float>(tutorial_rows);

        const float local_x = pixel.x - margin_x;
        const float local_y = pixel.y - margin_y;
        if (local_x < 0.0f || local_y < 0.0f)
        {
            return std::nullopt;
        }

        const std::size_t col = static_cast<std::size_t>(local_x / cell_w);
        const std::size_t row = static_cast<std::size_t>(local_y / cell_h);
        if (col >= tutorial_cols || row >= tutorial_rows)
        {
            return std::nullopt;
        }

        return row * tutorial_cols + col;
    }

    [[nodiscard]] bool tutorial_cells_are_adjacent(const std::size_t lhs, const std::size_t rhs) const noexcept
    {
        const auto lhs_row = lhs / tutorial_cols;
        const auto lhs_col = lhs % tutorial_cols;
        const auto rhs_row = rhs / tutorial_cols;
        const auto rhs_col = rhs % tutorial_cols;
        const auto row_delta = std::abs(static_cast<int>(lhs_row) - static_cast<int>(rhs_row));
        const auto col_delta = std::abs(static_cast<int>(lhs_col) - static_cast<int>(rhs_col));
        return (row_delta == 0 && col_delta == 1) || (row_delta == 1 && col_delta == 0);
    }

    [[nodiscard]] bool tutorial_drag_path_is_valid() const noexcept
    {
        if (tutorial_drag_path.empty() || tutorial_prize_path.empty())
        {
            return false;
        }

        if (tutorial_drag_path.size() != tutorial_prize_path.size())
        {
            return false;
        }

        if (tutorial_drag_path.front() != tutorial_start_cell || tutorial_drag_path.back() != tutorial_goal_cell)
        {
            return false;
        }

        for (std::size_t i = 0; i < tutorial_drag_path.size(); ++i)
        {
            if (tutorial_drag_path[i] != tutorial_prize_path[i])
            {
                return false;
            }
        }

        return tutorial_bad_cells.empty();
    }

    [[nodiscard]] static bool should_process_path_drag() noexcept
    {
        return IS_APP_RUNNING(AppState::TUTORIAL) || IS_APP_RUNNING(AppState::PLAYING);
    }

    void begin_path_drag_if_possible(const sf::Event::MouseButtonPressed &mouse)
    {
        if (!should_process_path_drag() || mouse.button != sf::Mouse::Button::Left)
        {
            return;
        }

        const auto pos = scene->mapPixelToCoords({mouse.position.x, mouse.position.y});
        const auto clicked = tutorial_cell_from_pixel(pos);
        if (!clicked.has_value() || *clicked != tutorial_start_cell)
        {
            return;
        }

        tutorial_drag_active = true;
        tutorial_drag_path = {*clicked};
        tutorial_prompt = "Keep dragging to the green prize.";
        if (tutorial_prompt_text.has_value())
        {
            tutorial_prompt_text->setString(tutorial_prompt);
        }
    }

    void continue_path_drag_if_active(const sf::Event::MouseMoved &mouse)
    {
        if (!should_process_path_drag() || !tutorial_drag_active)
        {
            return;
        }

        const auto pos = scene->mapPixelToCoords({mouse.position.x, mouse.position.y});
        update_tutorial_drag_path(pos);
    }

    void end_path_drag_if_active(const sf::Event::MouseButtonReleased &mouse)
    {
        if (!should_process_path_drag() || !tutorial_drag_active || mouse.button != sf::Mouse::Button::Left)
        {
            return;
        }

        const auto pos = scene->mapPixelToCoords({mouse.position.x, mouse.position.y});
        update_tutorial_drag_path(pos);
        finish_tutorial_drag();
    }

    void on_valid_drag_path_complete()
    {
        if (IS_APP_RUNNING(AppState::PLAYING))
        {
            ++playing_streak;
            const int streak_bonus = std::min(playing_streak - 1, 10);
            playing_score += 10 + (streak_bonus * 2);
            update_score_overlay();

            tutorial_prompt = "Path complete! Keep chaining clean runs for streak bonus.";
            tutorial_drag_path.clear();
            tutorial_bad_cells.clear();
            if (tutorial_prompt_text.has_value())
            {
                tutorial_prompt_text->setString(tutorial_prompt);
            }

            if (scene->interaction_sfx_loaded)
            {
                scene->interaction_sfx.play();
            }

            reset_tutorial_scene();
            return;
        }

        tutorial_prompt = "Nice! You found the prize. Press B for a fresh traversal.";
        if (tutorial_prompt_text.has_value())
        {
            tutorial_prompt_text->setString(tutorial_prompt);
        }
        begin_transition_to_playing();
    }

    void update_tutorial_drag_path(const sf::Vector2f pixel)
    {
        const auto cell = tutorial_cell_from_pixel(pixel);
        if (!cell.has_value())
        {
            return;
        }

        const auto index = *cell;
        if (tutorial_drag_path.empty())
        {
            if (index == tutorial_start_cell)
            {
                tutorial_drag_path = {index};
            }
            return;
        }

            if (index == tutorial_drag_path.back())
        {
            return;
        }

        if (std::ranges::find(tutorial_drag_path, index) != tutorial_drag_path.cend())
        {
            return;
        }

        tutorial_drag_path.push_back(index);
        if (!tutorial_cell_is_on_prize_path(index) && std::ranges::find(tutorial_bad_cells, index) == tutorial_bad_cells.end())
        {
            tutorial_bad_cells.push_back(index);
        }
    }

    void finish_tutorial_drag()
    {
        tutorial_drag_active = false;
        const bool valid_path = tutorial_drag_path_is_valid();

        if (valid_path)
        {
            on_valid_drag_path_complete();
            return;
        }

        tutorial_prompt = "Try again: begin at the glowing tile, stay on the hidden prize path, and finish on the green prize.";
        if (IS_APP_RUNNING(AppState::PLAYING))
        {
            playing_streak = 0;
            playing_score = std::max(0, playing_score - 4);
            tutorial_prompt = "Path failed. Streak reset and a small score penalty applied.";
            update_score_overlay();
        }

        tutorial_drag_path.clear();
        tutorial_bad_cells.clear();
        if (tutorial_prompt_text.has_value())
        {
            tutorial_prompt_text->setString(tutorial_prompt);
        }
    }

    void draw_tutorial_scene(const bool show_guidance_text = true)
    {
        draw_tutorial_scene(*scene, show_guidance_text);
    }

    void draw_tutorial_scene(sf::RenderTarget &target, const bool show_guidance_text = true)
    {
        if (!tutorial_prompt_text.has_value())
        {
            return;
        }

        const auto area_size = scene->getSize();
        const float margin_x = 28.0f;
        const float margin_y = 60.0f;
        const float cell_w = (static_cast<float>(area_size.x) - margin_x * 2.0f) / static_cast<float>(tutorial_cols);
        const float cell_h = (static_cast<float>(area_size.y) - margin_y * 2.0f) / static_cast<float>(tutorial_rows);

        tutorial_cells.clear();
        tutorial_cells.reserve(tutorial_rows * tutorial_cols);

        for (std::size_t row = 0; row < tutorial_rows; ++row)
        {
            for (std::size_t col = 0; col < tutorial_cols; ++col)
            {
                sf::RectangleShape cell{{cell_w - 2.0f, cell_h - 2.0f}};
                cell.setPosition({margin_x + static_cast<float>(col) * cell_w + 1.0f,
                                  margin_y + static_cast<float>(row) * cell_h + 1.0f});
                cell.setFillColor(sf::Color(26, 32, 50));
                cell.setOutlineColor(sf::Color(70, 82, 96));
                cell.setOutlineThickness(1.0f);
                tutorial_cells.push_back(cell);
            }
        }

        for (const auto &cell : tutorial_cells)
        {
            target.draw(cell);
        }

        for (const auto index : tutorial_drag_path)
        {
            const auto col = index % tutorial_cols;
            const auto row = index / tutorial_cols;
            sf::RectangleShape trail{{cell_w - 6.0f, cell_h - 6.0f}};
            trail.setPosition({margin_x + static_cast<float>(col) * cell_w + 3.0f,
                               margin_y + static_cast<float>(row) * cell_h + 3.0f});
            const bool is_prize_cell = tutorial_cell_is_on_prize_path(index);
            trail.setFillColor(is_prize_cell ? sf::Color(90, 164, 255, 170) : sf::Color(220, 58, 58, 200));
            trail.setOutlineColor(is_prize_cell ? sf::Color(180, 220, 255) : sf::Color(255, 180, 180));
            trail.setOutlineThickness(1.3f);
            target.draw(trail);
        }

        const auto start_index = tutorial_start_cell;
        const auto goal_index = tutorial_goal_cell;
        const auto start_col = start_index % tutorial_cols;
        const auto start_row = start_index / tutorial_cols;
        const auto goal_col = goal_index % tutorial_cols;
        const auto goal_row = goal_index / tutorial_cols;

        sf::RectangleShape glow_start{{cell_w - 4.0f, cell_h - 4.0f}};
        glow_start.setPosition({margin_x + static_cast<float>(start_col) * cell_w + 2.0f,
                                margin_y + static_cast<float>(start_row) * cell_h + 2.0f});
        glow_start.setFillColor(sf::Color(112, 124, 255, 220));
        glow_start.setOutlineColor(sf::Color(170, 187, 255));
        glow_start.setOutlineThickness(1.5f);
        target.draw(glow_start);

        sf::RectangleShape glow_goal{{cell_w - 4.0f, cell_h - 4.0f}};
        glow_goal.setPosition({margin_x + static_cast<float>(goal_col) * cell_w + 2.0f,
                               margin_y + static_cast<float>(goal_row) * cell_h + 2.0f});
        glow_goal.setFillColor(sf::Color(60, 180, 110, 220));
        glow_goal.setOutlineColor(sf::Color(170, 255, 200));
        glow_goal.setOutlineThickness(1.5f);
        target.draw(glow_goal);

        if (show_guidance_text)
        {
            tutorial_prompt_text->setPosition({14.f, static_cast<float>(area_size.y) - 42.f});
            target.draw(*tutorial_prompt_text);
        }
    }

    void update_apply_timing_overlay()
    {
        if (!apply_timing_text)
        {
            return;
        }

        const auto ms = fmt::format("{:.2f}", how_long_last_apply_took);
        fmt::println("Apply took {} ms\n", ms);

        const auto bounds = apply_timing_text->getLocalBounds();
        const float x = 10.0f;
        const float y = static_cast<float>(scene->getSize().y) - bounds.size.y - 12.0f;
        apply_timing_text->setPosition({x, y});
        update_state_overlay();
    }
    [[nodiscard]] static std::string_view current_state_label() noexcept
    {
        if (IS_APP_RUNNING(AppState::TUTORIAL))
        {
            return "TUTORIAL";
        }
        if (IS_APP_RUNNING(AppState::TRANSITION))
        {
            return "TRANSITION";
        }
        if (IS_APP_RUNNING(AppState::PLAYING))
        {
            return "PLAYING";
        }
        if (IS_APP_RUNNING(AppState::MENU))
        {
            return "MENU";
        }
        return "UNKNOWN";
    }

    void update_state_overlay()
    {
        if (!state_label_text)
        {
            return;
        }

        state_label_text->setString(fmt::format("State: {}", current_state_label()));
        state_label_text->setPosition({10.0f, 10.0f});
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
        const b2Vec2 impulse{dx * inv_len * impulse_scale, dy * inv_len * impulse_scale};
        b2Body_ApplyLinearImpulseToCenter(physics_balls[ball_index].body, impulse, true);
    }

    void rebuild_solution_path()
    {
    }

    void create_world()
    {
        if (B2_IS_NON_NULL(world_with_physics))
        {
            b2DestroyWorld(world_with_physics);
        }

        b2WorldDef def = b2DefaultWorldDef();
        def.gravity = {0.0f, 0.0f};
        world_with_physics = b2CreateWorld(&def);
        player_body = b2_nullBodyId;
        physics_wall_bodies.clear();
        physics_balls.clear();
        grabbed_ball_index.reset();
        grabbed_ball_start_position.reset();
    }

    b2Vec2 px_to_m(const float x, const float y) const
    {
        return {x / scene->pixels_per_meter, y / scene->pixels_per_meter};
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
        return {p.x * scene->pixels_per_meter * world_scale_x, p.y * scene->pixels_per_meter * world_scale_y};
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
        const b2Polygon box = b2MakeBox((w * 0.5f) / scene->pixels_per_meter, (h * 0.5f) / scene->pixels_per_meter);
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
        const b2Circle circle = {{0.0f, 0.0f}, dynamic_ball::ball_radius_px / scene->pixels_per_meter};
        b2CreateCircleShape(body, &shape_def, &circle);

        dynamic_ball ball{sfml_font};
        ball.body = body;
        ball.value = random_value_ball();
        ball.drawable.setString(std::to_string(ball.value));
        ball.drawable.setCharacterSize(static_cast<unsigned int>(dynamic_ball::ball_radius_px * 2.2f));
        ball.drawable.setFillColor(value_ball_tint(ball.value));
        ball.drawable.setOutlineColor(sf::Color(35, 26, 18));
        ball.drawable.setOutlineThickness(1.5f);
        ball.drawable.setStyle(sf::Text::Style::Bold);
        ball.drawable.setOrigin({static_cast<float>(ball.drawable.getCharacterSize()) * 0.5f,
                                 static_cast<float>(ball.drawable.getCharacterSize()) * 0.5f});
        ball.drawable.setPosition(position);
        physics_balls.push_back(ball);
    }

    [[nodiscard]] std::optional<std::size_t> find_ball_at(const sf::Vector2f pos_pixels) const
    {
        const b2Vec2 target = screen_px_to_world_m(pos_pixels.x, pos_pixels.y);
        float best_dist_sq = 1e9f;
        std::optional<std::size_t> best_index;

        for (std::size_t i = 0; i < physics_balls.size(); ++i)
        {
            if (physics_balls[i].collected)
            {
                continue;
            }

            const b2Vec2 p = b2Body_GetPosition(physics_balls[i].body);
            const float dx = target.x - p.x;
            const float dy = target.y - p.y;
            const float d2 = dx * dx + dy * dy;
            const auto max_pick_radius_m = (dynamic_ball::ball_radius_px * 2.2f) / scene->pixels_per_meter;
            const float max_pick_dist_sq = max_pick_radius_m * max_pick_radius_m;
            if (d2 <= max_pick_dist_sq && d2 < best_dist_sq)
            {
                best_dist_sq = d2;
                best_index = i;
            }
        }

        return best_index;
    }

    [[nodiscard]] static int random_value_ball() noexcept
    {
        const bool negative = RNG.get_int(0, 9) == 0;
        const int magnitude = RNG.get_int(1, 9);
        return negative ? -magnitude : magnitude;
    }

    [[nodiscard]] static sf::Color value_ball_tint(const int value) noexcept
    {
        if (value > 0)
        {
            return sf::Color(255, 210, 92);
        }
        if (value < 0)
        {
            return sf::Color(255, 112, 120);
        }
        return sf::Color(220, 224, 255);
    }

    void apply_number_ball_effect(std::size_t index, const sf::Vector2f pointer_pos, const sf::Vector2f swipe_delta)
    {
        if (index >= physics_balls.size() || physics_balls[index].collected)
        {
            return;
        }

        auto &ball = physics_balls[index];
        const sf::Vector2f ball_pos = ball.drawable.getPosition();
        const float radius = dynamic_ball::ball_radius_px * 1.75f;
        const float dist_sq = (pointer_pos.x - ball_pos.x) * (pointer_pos.x - ball_pos.x) +
                              (pointer_pos.y - ball_pos.y) * (pointer_pos.y - ball_pos.y);
        if (dist_sq > radius * radius)
        {
            return;
        }

        const int score_delta = ball.value;
        playing_score += score_delta;
        if (score_delta >= 0)
        {
            ++playing_streak;
        }
        else
        {
            playing_streak = 0;
        }
        update_score_overlay();

        const float swipe_mag = std::hypot(swipe_delta.x, swipe_delta.y);
        const b2Vec2 impulse_dir = [this, &ball, swipe_delta, swipe_mag]()
        {
            if (swipe_mag > 1.0e-4f)
            {
                return b2Vec2{swipe_delta.x / swipe_mag, swipe_delta.y / swipe_mag};
            }

            if (B2_IS_NON_NULL(player_body))
            {
                const b2Vec2 player_pos = b2Body_GetPosition(player_body);
                const b2Vec2 ball_pos_m = b2Body_GetPosition(ball.body);
                const float dx = player_pos.x - ball_pos_m.x;
                const float dy = player_pos.y - ball_pos_m.y;
                const float length = std::sqrt(dx * dx + dy * dy);
                if (length > 1.0e-4f)
                {
                    return b2Vec2{dx / length, dy / length};
                }
            }

            return b2Vec2{0.0f, -1.0f};
        }();

        if (B2_IS_NON_NULL(ball.body))
        {
            const float impulse_strength = 3.5f + std::abs(score_delta) * 0.9f;
            b2Body_ApplyLinearImpulseToCenter(ball.body, {impulse_dir.x * impulse_strength, impulse_dir.y * impulse_strength}, true);
        }

        ball.collected = true;
        ball.lifetime_seconds = static_cast<float>(RNG.get_int(300, 500)) / 100.0f;
        const auto base = value_ball_tint(ball.value);
        auto color = base;
        color.a = 255u;
        ball.drawable.setFillColor(color);
        ball.drawable.setOutlineColor(sf::Color(35, 30, 22));

        tutorial_prompt = fmt::format("{} {}!", score_delta >= 0 ? "Bonus" : "Penalty", score_delta);
        if (tutorial_prompt_text.has_value())
        {
            tutorial_prompt_text->setString(tutorial_prompt);
        }

        if (scene->interaction_sfx_loaded)
        {
            scene->interaction_sfx.play();
        }

        emit_bonus_particles(ball_pos);
    }

    void update_number_balls(const float dt)
    {
        for (auto it = physics_balls.begin(); it != physics_balls.end();)
        {
            auto &ball = *it;
            if (!ball.collected)
            {
                ++it;
                continue;
            }

            ball.lifetime_seconds -= dt;
            if (ball.lifetime_seconds <= 0.0f)
            {
                if (B2_IS_NON_NULL(ball.body))
                {
                    b2DestroyBody(ball.body);
                    ball.body = b2_nullBodyId;
                }
                it = physics_balls.erase(it);
                continue;
            }

            auto color = ball.drawable.getFillColor();
            const float fa = std::clamp(ball.lifetime_seconds / 5.0f, 0.0f, 1.0f);
            color.a = static_cast<std::uint8_t>(std::clamp(255.0f * fa, 0.0f, 255.0f));
            ball.drawable.setFillColor(color);
            ++it;
        }
    }

    void create_world_boundaries()
    {
        if (!B2_IS_NON_NULL(world_with_physics))
        {
            return;
        }

        const auto size = scene->getSize();
        const float width = static_cast<float>(size.x);
        const float height = static_cast<float>(size.y);
        const float t = std::max(8.0f, layout_wall_thickness * 2.0f);

        add_wall_body_from_rect(0.0f, 0.0f, width, t);
        add_wall_body_from_rect(0.0f, height - t, width, t);
        add_wall_body_from_rect(0.0f, 0.0f, t, height);
        add_wall_body_from_rect(width - t, 0.0f, t, height);
    }

    void build_geometry_and_physics()
    {
        ensure_playing_assets_ready();
        create_world();
        create_world_boundaries();

        const auto size = scene->getSize();
        if (size.x < 80u || size.y < 80u)
        {
            return;
        }

        const float margin = dynamic_ball::ball_radius_px * 3.0f;
        const int min_x = static_cast<int>(margin);
        const int max_x = static_cast<int>(static_cast<float>(size.x) - margin);
        const int min_y = static_cast<int>(margin);
        const int max_y = static_cast<int>(static_cast<float>(size.y) - margin);
        if (min_x >= max_x || min_y >= max_y)
        {
            return;
        }

        const auto num_balls = std::max<std::size_t>(6u, dynamic_ball::NUM_BALLS / 2u);
        for (std::size_t i = 0; i < num_balls; ++i)
        {
            const float x = static_cast<float>(RNG.get_int(min_x, max_x));
            const float y = static_cast<float>(RNG.get_int(min_y, max_y));
            add_ball({x, y});

            auto &ball = physics_balls.back();
            const float angle = static_cast<float>(RNG.get_int(0, 359)) * 3.14159265f / 180.0f;
            const float impulse = static_cast<float>(RNG.get_int(20, 55)) / 10.0f;
            b2Body_ApplyLinearImpulseToCenter(ball.body, {std::cos(angle) * impulse, std::sin(angle) * impulse}, true);
        }

        sync_ball_drawables();
    }

    void handle_events()
    {
        while (const auto event = scene->pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                scene->close();
            }

            if (const auto *mouse = event->getIf<sf::Event::MouseButtonPressed>())
            {
                const auto pos = scene->mapPixelToCoords({mouse->position.x, mouse->position.y});
                if (IS_APP_RUNNING(AppState::PLAYING) && mouse->button == sf::Mouse::Button::Left)
                {
                    if (const auto ball_index = find_ball_at(pos); ball_index.has_value())
                    {
                        apply_number_ball_effect(*ball_index, pos, {0.0f, 0.0f});
                        continue;
                    }
                }

                begin_path_drag_if_possible(*mouse);
            }

            if (const auto *mouse = event->getIf<sf::Event::MouseMoved>())
            {
                const auto pos = scene->mapPixelToCoords({mouse->position.x, mouse->position.y});
                if (IS_APP_RUNNING(AppState::PLAYING))
                {
                    static sf::Vector2f last_pointer_pos{};
                    static bool last_pointer_pos_valid{false};
                    const sf::Vector2f delta = last_pointer_pos_valid ? pos - last_pointer_pos : sf::Vector2f{0.0f, 0.0f};
                    last_pointer_pos = pos;
                    last_pointer_pos_valid = true;

                    for (std::size_t i = 0; i < physics_balls.size(); ++i)
                    {
                        if (!physics_balls[i].collected)
                        {
                            apply_number_ball_effect(i, pos, delta);
                            if (physics_balls[i].collected)
                            {
                                break;
                            }
                        }
                    }
                }
                continue_path_drag_if_active(*mouse);
            }

            if (const auto *mouse = event->getIf<sf::Event::MouseButtonReleased>())
            {
                end_path_drag_if_active(*mouse);
            }

            if (const auto *key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::Escape)
                {
                    scene->close();
                }
                else if (key->code == sf::Keyboard::Key::B)
                {
                    reset_tutorial_scene();
                }
                else if (key->code == sf::Keyboard::Key::H)
                {
                    should_show_info = !should_show_info;
                }
                else if (key->code == sf::Keyboard::Key::Space)
                {
                    focus_hold_active = true;
                }
            }

            if (const auto *key = event->getIf<sf::Event::KeyReleased>())
            {
                if (key->code == sf::Keyboard::Key::Space)
                {
                    focus_hold_active = false;
                }
            }

            if (const auto *resized = event->getIf<sf::Event::Resized>())
            {
                const float new_width = static_cast<float>(resized->size.x);
                const float new_height = static_cast<float>(resized->size.y);
                scene->setView(sf::View{{new_width * 0.5f, new_height * 0.5f}, {new_width, new_height}});
                update_screen_layout();
                update_apply_timing_overlay();
                update_score_overlay();
            }
        }
    }

    void step_physics(const float dt)
    {
        if (B2_IS_NON_NULL(world_with_physics))
        {
            if (B2_IS_NON_NULL(player_body))
            {
                const sf::Vector2f facing = sf::Vector2f{1.f, 1.f};
                constexpr float PLAYER_SPEED_MPS = 3.25f;
                constexpr float VELOCITY_BLEND = 0.24f;
                const b2Vec2 current_velocity = b2Body_GetLinearVelocity(player_body);
                const float move_speed = 0.2f;
                const b2Vec2 target_velocity{facing.x * move_speed, facing.y * move_speed};
                const b2Vec2 blended_velocity{
                    current_velocity.x + (target_velocity.x - current_velocity.x) * VELOCITY_BLEND,
                    current_velocity.y + (target_velocity.y - current_velocity.y) * VELOCITY_BLEND};
                b2Body_SetLinearVelocity(player_body, blended_velocity);
                b2Body_SetAwake(player_body, true);
            }

            b2World_Step(world_with_physics, dt, 4);
        }
    }

    void sync_ball_drawables()
    {
        std::ranges::for_each(physics_balls, [this](dynamic_ball &ball)
                              {
                if (!B2_IS_NON_NULL(ball.body))
                {
                    return;
                }
                const b2Vec2 p = b2Body_GetPosition(ball.body);
                ball.drawable.setPosition(world_m_to_screen_px(p));
                ball.drawable.setScale({ world_scale_x, world_scale_y }); });
    }

    [[nodiscard]] static float player_radius_in_pixels() noexcept
    {
        return dynamic_ball::ball_radius_px * 2.22f;
    }
};

int main()
{
    try
    {
#if defined(MAZE_DEBUG)

        amazing_sfml_app::run_self_tests();
#endif

        amazing_sfml_app app{};
        app.run();
    }
    catch (const std::exception &ex)
    {
        fmt::print(stderr, "Unhandled exception: {}\n", ex.what());
    }
    return EXIT_SUCCESS;
}
