#include "player.h"

#include <chrono>
#include <ranges>

#include "command_queue.h"
#include "entity.h"
#include "item.h"
#include "matrix.h"
#include "world.h"

#include <SDL3/SDL.h>

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/enums.h>
#include <MazeBuilder/grid.h>
#include <MazeBuilder/grid_factory.h>
#include <MazeBuilder/grid_interface.h>
#include <MazeBuilder/grid_operations.h>
#include <MazeBuilder/objectify.h>
#include <MazeBuilder/pixels.h>
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/stringify.h>
#include <MazeBuilder/string_utils.h>
#include <MazeBuilder/wavefront_object_helper.h>

constexpr auto DAY_LENGTH = 600;
constexpr auto DEFAULT_FOV = 65.0f;
constexpr auto DEFAULT_ORTHO = 0u;
constexpr auto ORTHO_ENABLED_VAL = 64;
constexpr auto SCROLL_THRESHOLD = 0.1f;
constexpr auto ZOOM_FOV = 15.f;

player::player()
    : scene_node{}
      , m_is_active{true}
      , m_on_ground{false}
      , m_is_flying{false}
      , m_is_ctrl_held{false}
      , m_name{"zm"}
      , m_buffer{}
      , m_item_index{0}
      , m_world{nullptr}
      , m_maze_task{
          [this](const mazes::configurator& config)-> std::unique_ptr<mazes::grid_interface>
          {
              const auto a = mazes::configurator::make_algo_from_config(this->m_configs.maze);
              auto g = std::make_unique<mazes::grid>(config.rows(), config.columns(), config.levels());
              if (!a.has_value())
              {
                  return g;
              }
              thread_local mazes::randomizer rng{};
              rng.seed(config.seed());
              if (!a.value()->run(g.get(), std::ref(rng)))
              {
                  return nullptr;
              }

              if (thread_local mazes::stringify stringifier{}; !stringifier.run(g.get(), std::ref(rng)))
              {
                  return nullptr;
              }

              if (thread_local mazes::objectify obj_tool{}; !obj_tool.run(g.get(), std::ref(rng)))
              {
                  return nullptr;
              }

              if (thread_local mazes::wavefront_object_helper woh{}; !woh.run(g.get(), std::ref(rng)))
              {
                  return nullptr;
              }

              return std::move(g);
          }
      }
      , m_maze_future{}
      , m_grid_factory{std::make_unique<mazes::grid_factory>()}
{
    m_grid_factory->register_creator(m_name, m_maze_task);

    set_category(Entity::PLAYER);

    // Movement key bindings
    m_key_binding[SDL_SCANCODE_A] = PlayerAction::MOVE_LEFT;
    m_key_binding[SDL_SCANCODE_E] = PlayerAction::PREVIEW_MAZE;
    m_key_binding[SDL_SCANCODE_D] = PlayerAction::MOVE_RIGHT;
    m_key_binding[SDL_SCANCODE_F] = PlayerAction::BUILD_MAZE;
    m_key_binding[SDL_SCANCODE_S] = PlayerAction::MOVE_BACKWARD;
    m_key_binding[SDL_SCANCODE_T] = PlayerAction::TAG_SIGN;
    m_key_binding[SDL_SCANCODE_W] = PlayerAction::MOVE_FORWARD;
    m_key_binding[SDL_SCANCODE_SPACE] = PlayerAction::JUMP;
    m_key_binding[SDL_SCANCODE_LSHIFT] = PlayerAction::MOVE_DOWN;
    m_key_binding[SDL_SCANCODE_RSHIFT] = PlayerAction::MOVE_UP;
    m_key_binding[SDL_SCANCODE_TAB] = PlayerAction::FLY;

    m_configs.day_length = DAY_LENGTH;
    m_configs.start_time = DAY_LENGTH / 2 * 1000;
    m_configs.start_ticks = SDL_GetTicks();
    m_configs.fov = DEFAULT_FOV;
    m_configs.ortho = DEFAULT_ORTHO;
    m_configs.invert_mouse = false;
    m_configs.tag = "put maze here";

    initialize_actions();

    // Set category for all player actions
    for (auto& [_, category] : m_action_binding | std::views::values)
    {
        category = Entity::PLAYER;
    }
}

void player::handle_event(const SDL_Event& event, command_queue& commands) noexcept
{
    if (event.type == SDL_EVENT_QUIT)
    {
        m_is_active = false;
    }
    if (event.type == SDL_EVENT_MOUSE_WHEEL)
    {
        if (event.wheel.y > SCROLL_THRESHOLD)
        {
            // Scroll up
            if (m_item_index > 0)
            {
                m_item_index--;
            }
            else
            {
                m_item_index = item::items.size() - 1;
            }
        }
        else if (event.wheel.y < -SCROLL_THRESHOLD)
        {
            // Scroll down
            if (m_item_index + 1 < item::items.size())
            {
                m_item_index++;
            }
            else
            {
                m_item_index = 0;
            }
        }
    }
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        // Track Left Control modifier key
        if (event.key.scancode == SDL_SCANCODE_LCTRL)
        {
            m_is_ctrl_held = true;
        }

        if (const auto found = m_key_binding.find(event.key.scancode);
            found != m_key_binding.cend() && !is_realtime_action(found->second))
        {
            if (found->second == PlayerAction::JUMP)
            {
                // Only allow jumping when on ground
                if (m_on_ground)
                {
                    commands.push(m_action_binding[found->second]);
                }
                return;
            }
            commands.push(m_action_binding[found->second]);
        }
    }
    if (event.type == SDL_EVENT_KEY_UP)
    {
        if (event.key.scancode == SDL_SCANCODE_LCTRL)
        {
            m_is_ctrl_held = false;
        }
    }
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (event.button.button == SDL_BUTTON_LEFT)
        {
            if (m_is_ctrl_held)
            {
                commands.push(m_action_binding[PlayerAction::PLACE_LIGHT]);
            }
            else
            {
                commands.push(m_action_binding[PlayerAction::DESTROY_BLOCK]);
            }
        }
        else if (event.button.button == SDL_BUTTON_RIGHT)
        {
            commands.push(m_action_binding[PlayerAction::BUILD_BLOCK]);
        }
        else if (event.button.button == SDL_BUTTON_MIDDLE)
        {
            // Keep existing behavior: copy block type
            on_middle_click();
        }
    }
    if (event.type == SDL_EVENT_MOUSE_MOTION)
    {
        constexpr float mouse_sensitivity = 0.0025f;
        position* player_pos = &this->pos;
        player_pos->rx += event.motion.xrel * mouse_sensitivity;

        if (this->m_configs.invert_mouse)
        {
            player_pos->ry += event.motion.yrel * mouse_sensitivity;
        }
        else
        {
            player_pos->ry -= event.motion.yrel * mouse_sensitivity;
        }

        // Keep rotation within bounds
        if (player_pos->rx < 0)
        {
            player_pos->rx += RADIANS(360.0);
        }
        if (player_pos->rx >= RADIANS(360.0))
        {
            player_pos->rx -= RADIANS(360.0);
        }
        player_pos->ry = SDL_max(player_pos->ry, -RADIANS(90.0));
        player_pos->ry = SDL_min(player_pos->ry, RADIANS(90.0));
    }
}

void player::draw() const noexcept
{
}

void player::handle_realtime_input(command_queue& commands)
{
    static int frame_counter = 0;
    bool any_key_pressed = false;

    for (auto& [id, action] : m_key_binding)
    {
        // Regular realtime actions OR JUMP when flying
        if (is_realtime_action(action) || (action == PlayerAction::JUMP && m_is_flying))
        {
            int numKeys = 0;

            if (const auto* keyState = SDL_GetKeyboardState(&numKeys);
                keyState && id < static_cast<std::uint32_t>(numKeys) && keyState[id])
            {
                commands.push(m_action_binding[action]);
                any_key_pressed = true;
            }
        }
    }

    if (any_key_pressed)
    {
        frame_counter++;
    }

    // Update projected plane position every frame if maze is ready
    if (m_world && m_configs.maze_ready)
    {
        int hx, hy, hz, face;
        if (m_world->hit_test_face(&hx, &hy, &hz, &face))
        {
            // Valid target found - update plane state
            bool face_changed = (m_projected_plane.target_face != face) || !m_projected_plane.visible;

            m_projected_plane.visible = true;
            m_projected_plane.texture_id = m_configs.maze_texture_id;
            m_projected_plane.target_x = hx;
            m_projected_plane.target_y = hy;
            m_projected_plane.target_z = hz;
            m_projected_plane.target_face = face;
            m_projected_plane.has_valid_target = true;
        }
        else
        {
            // No valid target (looking at sky/void) - hide plane gracefully
            m_projected_plane.has_valid_target = false;
            m_projected_plane.visible = false;
        }
    }
    else
    {
        // Maze not ready - ensure plane is hidden
        m_projected_plane.visible = false;
        m_projected_plane.has_valid_target = false;
    }
}

void player::assign_key(const PlayerAction action, const std::uint32_t key)
{
    // Remove all keys that already map to action
    for (auto it = m_key_binding.begin(); it != m_key_binding.end();)
    {
        if (it->second == action)
        {
            it = m_key_binding.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Insert new binding
    m_key_binding.insert_or_assign(key, action);
}

[[nodiscard]] std::uint32_t player::get_assigned_key(const PlayerAction action) const
{
    for (const auto& [f, s] : m_key_binding)
    {
        if (s == action)
            return f;
    }

    return SDL_SCANCODE_UNKNOWN;
}

bool player::is_active() const noexcept
{
    return m_is_active;
}

void player::set_active(const bool active) noexcept
{
    m_is_active = active;
}

bool player::is_flying() const noexcept
{
    return m_is_flying;
}

void player::set_flying(const bool flying) noexcept
{
    this->m_is_flying = flying;
}

bool player::is_on_ground() const noexcept
{
    return m_on_ground;
}

void player::set_on_ground(const bool grounded) noexcept
{
    this->m_on_ground = grounded;
}

std::uint32_t player::get_buffer() const noexcept
{
    return this->m_buffer;
}

void player::set_buffer(const std::uint32_t value) noexcept
{
    this->m_buffer = value;
}

std::int32_t player::get_item() const noexcept
{
    if (this->m_item_index >= 0 && this->m_item_index < item::items.size())
    {
        return item::items.at(this->m_item_index);
    }
    return -1;
}

void player::set_item(const std::int32_t value) noexcept
{
    if (value >= 0 && value < item::items.size())
    {
        this->m_item_index = value;
    }
}

std::string player::get_name() const noexcept
{
    return m_name;
}

void player::set_name(const std::string& name) noexcept
{
    m_name = name;
}

void player::set_world(world* w) noexcept
{
    m_world = w;
}

std::string player::get_local_time() const noexcept
{
    if (!m_world)
    {
        return "00:00";
    }

    // time_of_day() returns 0.0-1.0 representing position in the day cycle
    const float time_fraction = m_world->time_of_day();
    const float total_hours = time_fraction * 24.0f;

    // Extract hours and minutes
    int hour = static_cast<int>(total_hours);
    const int minute = static_cast<int>((total_hours - static_cast<float>(hour)) * 60.0f);

    // Convert to 12-hour format
    const std::string_view am_pm = hour < 12 ? "am" : "pm";
    hour = hour % 12;
    // Convert 0 to 12 for midnight/noon
    hour = hour ? hour : 12;

    return std::string{mazes::string_utils::format("{}:{:02d}{}", hour, minute, am_pm)};
}

const player::projected_plane& player::get_projected_plane() const noexcept
{
    return m_projected_plane;
}

bool player::run(mazes::grid_interface* g, mazes::randomizer& rng) const noexcept
{
    if (!g)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "player::run - grid is nullptr\n");
        return false;
    }

    // Use m_maze_task which already generates and processes the maze
    auto result_grid = m_maze_task(m_configs.maze);

    if (!result_grid)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "player::run - maze task failed\n");
        return false;
    }

    // Copy the generated maze data to the provided grid
    *g = *result_grid;

    return true;
}

void player::initialize_actions()
{
    // Movement parameters for smooth interpolation
    constexpr float max_move_speed = 5.0f;
    constexpr float acceleration = 0.2f;

    m_action_binding[PlayerAction::MOVE_BACKWARD].action = derived_action<player>(
        [](player& p, const float dt, mazes::randomizer& rng)
        {
            const float target_vx = -SDL_sinf(p.pos.rx) * max_move_speed;
            const float target_vz = SDL_cosf(p.pos.rx) * max_move_speed;

            p.vel.vx = lerp(p.vel.vx, target_vx, acceleration);
            p.vel.vz = lerp(p.vel.vz, target_vz, acceleration);

            const float dt_seconds = dt / 1000.0f;
            p.pos.x += p.vel.vx * dt_seconds;
            p.pos.z += p.vel.vz * dt_seconds;
        });

    m_action_binding[PlayerAction::MOVE_FORWARD].action = derived_action<player>(
        [](player& p, const float dt, mazes::randomizer& rng)
        {
            const float target_vx = SDL_sinf(p.pos.rx) * max_move_speed;
            const float target_vz = -SDL_cosf(p.pos.rx) * max_move_speed;

            p.vel.vx = lerp(p.vel.vx, target_vx, acceleration);
            p.vel.vz = lerp(p.vel.vz, target_vz, acceleration);

            const float dt_seconds = dt / 1000.0f;
            p.pos.x += p.vel.vx * dt_seconds;
            p.pos.z += p.vel.vz * dt_seconds;
        });

    m_action_binding[PlayerAction::MOVE_LEFT].action = derived_action<player>(
        [](player& p, const float dt, mazes::randomizer& rng)
        {
            const float target_vx = -SDL_cosf(p.pos.rx) * max_move_speed;
            const float target_vz = -SDL_sinf(p.pos.rx) * max_move_speed;

            p.vel.vx = lerp(p.vel.vx, target_vx, acceleration);
            p.vel.vz = lerp(p.vel.vz, target_vz, acceleration);

            const float dt_seconds = dt / 1000.0f;
            p.pos.x += p.vel.vx * dt_seconds;
            p.pos.z += p.vel.vz * dt_seconds;
        });

    m_action_binding[PlayerAction::MOVE_RIGHT].action = derived_action<player>(
        [](player& p, const float dt, mazes::randomizer& rng)
        {
            const float target_vx = SDL_cosf(p.pos.rx) * max_move_speed;
            const float target_vz = SDL_sinf(p.pos.rx) * max_move_speed;

            p.vel.vx = lerp(p.vel.vx, target_vx, acceleration);
            p.vel.vz = lerp(p.vel.vz, target_vz, acceleration);

            const float dt_seconds = dt / 1000.0f;
            p.pos.x += p.vel.vx * dt_seconds;
            p.pos.z += p.vel.vz * dt_seconds;
        });

    m_action_binding[PlayerAction::JUMP].action = derived_action<player>(
        [](player& p, float dt, mazes::randomizer& rng)
        {
            if (p.m_is_flying)
            {
                // In flying mode, move up
                constexpr float flySpeed = 0.15f;
                p.vel.vy = flySpeed;
            }
            else if (p.m_on_ground)
            {
                // Normal jump when on ground
                constexpr float jumpVelocity = 8.0f;
                p.vel.vy = jumpVelocity;
                p.m_on_ground = false;
            }
        });

    m_action_binding[PlayerAction::TAG_SIGN].action = derived_action<player>(
        [this](player& p, float dt, mazes::randomizer& rng)
        {
            if (m_world)
            {
                on_tag_sign();
            }
        });

    m_action_binding[PlayerAction::MOVE_DOWN].action = derived_action<player>(
        [](player& p, float dt, mazes::randomizer& rng)
        {
            if (p.m_is_flying)
            {
                // In flying mode, move down
                constexpr float flySpeed = 4.85f;
                p.vel.vy = -flySpeed;
            }
        });

    m_action_binding[PlayerAction::MOVE_UP].action = derived_action<player>(
        [](player& p, float dt, mazes::randomizer& rng)
        {
            if (p.m_is_flying)
            {
                // In flying mode, move down
                constexpr float flySpeed = 4.85f;
                p.vel.vy = flySpeed;
            }
        });

    m_action_binding[PlayerAction::FLY].action = derived_action<player>(
        [](player& p, float dt, mazes::randomizer& rng)
        {
            p.m_is_flying = !p.m_is_flying;

            if (p.m_is_flying)
            {
                p.vel.vy = 0.0f;
            }
        });

    m_action_binding[PlayerAction::BUILD_BLOCK].action = derived_action<player>(
        [this](const player& p, float dt, mazes::randomizer& rng)
        {
            if (p.m_world)
            {
                on_right_click();
            }
        });

    m_action_binding[PlayerAction::DESTROY_BLOCK].action = derived_action<player>(
        [this](const player& p, float dt, mazes::randomizer& rng)
        {
            if (p.m_world)
            {
                on_left_click();
            }
        });

    m_action_binding[PlayerAction::PLACE_LIGHT].action = derived_action<player>(
        [this](const player& p, float dt, mazes::randomizer& rng)
        {
            if (p.m_world)
            {
                on_light();
            }
        });

    m_action_binding[PlayerAction::PREVIEW_MAZE].action = derived_action<player>(
        [](player& p, const float dt, mazes::randomizer& rng)
        {
            if (p.m_configs.preview_enabled && p.m_world)
            {
                // Check cooldown before generating
                if (!p.is_maze_generation_ready())
                {
                    const auto remaining_ms = p.get_maze_cooldown_remaining_ms();
                    const auto remaining_seconds = (remaining_ms + 999) / 1000; // Round up
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                               "Maze generation on cooldown. Please wait %.1f seconds",
                               static_cast<float>(remaining_ms) / 1000.0f);
                    return;
                }

                // Generate the maze texture
                if (p.generate_maze_texture(rng))
                {
                    // Update cooldown timestamp on successful generation
                    p.m_last_maze_generation_time = SDL_GetTicks();

                    SDL_Log("Maze texture generated successfully! Texture ID: %u", p.m_configs.maze_texture_id);
                    SDL_Log("Maze preview will appear on block faces as you move your crosshair");
                    SDL_Log("Next generation available in %llu seconds",
                           player::MAZE_GENERATION_COOLDOWN_MS / 1000);
                }
                else
                {
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to generate maze texture");
                }
            }
        });

    m_action_binding[PlayerAction::BUILD_MAZE].action = derived_action<player>(
        [](player& p, const float dt, mazes::randomizer& rng)
        {
            if (p.m_configs.preview_enabled && p.m_world)
            {
            }
        });
}

bool player::is_realtime_action(const PlayerAction action) noexcept
{
    switch (action)
    {
    case PlayerAction::MOVE_LEFT:
    case PlayerAction::MOVE_RIGHT:
    case PlayerAction::MOVE_FORWARD:
    case PlayerAction::MOVE_BACKWARD:
    case PlayerAction::MOVE_DOWN:
    case PlayerAction::MOVE_UP:
        return true;
    default:
        return false;
    }
}

void player::on_light() const noexcept
{
    const position* s = &this->pos;
    int hx, hy, hz;
    if (const int hw = m_world->hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        hy > 0 && hy < item::TOTAL_BLOCKS && item::is_destructable(hw))
    {
        m_world->toggle_light(hx, hy, hz);
    }
}

void player::on_left_click() const noexcept
{
    const position* s = &this->pos;
    int hx, hy, hz;
    if (const auto hw = m_world->hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        hy > 0 && hy < 256 && item::is_destructable(hw))
    {
        m_world->set_block(hx, hy, hz, 0);
        world::record_block(hx, hy, hz, 0);
#if defined(MAZE_DEBUG)
        SDL_Log("on_left_click(%d, %d, %d, %d, block_type: %d): ", hx, hy, hz, hw, get_item());
#endif
        if (item::is_plant(m_world->get_block(hx, hy + 1, hz)))
        {
            m_world->set_block(hx, hy + 1, hz, 0);
        }
    }
}

void player::on_right_click() const noexcept
{
    const position* s = &this->pos;
    int hx, hy, hz;
    if (const int hw = m_world->hit_test(1, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        hy > 0 && hy < item::TOTAL_BLOCKS && item::is_obstacle(hw))
    {
        if (!world::player_intersects_block(2, s->x, s->y, s->z, hx, hy, hz))
        {
            m_world->set_block(hx, hy, hz, get_item());
            world::record_block(hx, hy, hz, get_item());
#if defined(MAZE_DEBUG)
            SDL_Log("on_right_click(%d, %d, %d, %d, block_type: %d): ", hx, hy, hz, hw,
                    get_item());
#endif
        }
    }
}

void player::on_middle_click() noexcept
{
    const position* s = &this->pos;
    int hx, hy, hz;
    const int hw = m_world->hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    for (int i = 0; i < item::items.size(); i++)
    {
        if (item::items.at(i) == hw)
        {
            this->m_item_index = i;
#if defined(MAZE_DEBUG)
            SDL_Log("Copying item index: %d\n", i);
#endif
            break;
        }
    }
}

void player::on_tag_sign() const noexcept
{
    int hx, hy, hz, face;
    if (auto result = m_world->hit_test_face(&hx, &hy, &hz, &face))
    {
        m_world->set_sign(hx, hy, hz, face, m_configs.tag);
    }
}

float player::lerp(float a, float b, float t) noexcept
{
    return a + t * (b - a);
}

bool player::generate_maze_texture(mazes::randomizer& rng) noexcept
{
    try
    {
        if (const auto& g = m_grid_factory->create(m_name, m_configs.maze);
            g.has_value())
        {
            auto&& val = g.value();

            val->operations().set_str("");
            // Convert to pixel representation
            if (const mazes::pixels pixel_converter; !pixel_converter.run(val.get(), rng))
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to convert maze to pixels\n");
                return false;
            }

            // Get the pixel data
            const auto pixel_data = val->operations().get_pixels();
            if (pixel_data.empty())
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Pixel data is empty\n");
                return false;
            }

            // Calculate dimensions from actual pixel data
            // pixels.cpp creates RGBA data (4 bytes per pixel) with dimensions based on actual ASCII string lengths
            auto [rows, columns, _] = val->operations().get_dimensions();

            // Calculate scale (same as in pixels.cpp)
            constexpr unsigned int MIN_SCALE = 1;
            constexpr unsigned int MAX_SCALE = 10;
            const auto calculated_scale = static_cast<unsigned int>(SDL_sqrtf(rows * columns));
            const auto scale = std::clamp(calculated_scale, MIN_SCALE, MAX_SCALE);

            // Height is predictable: (rows*2+1) * scale
            const auto ascii_height = rows * 2 + 1;
            const int height = static_cast<int>(ascii_height * scale);

            // Width must be calculated from pixel data size since ASCII lines may have varying lengths
            // pixel_data.size() = width * height * 4 (RGBA)
            const int width = static_cast<int>(pixel_data.size() / (height * 4));

            SDL_Log("Maze pixel data: %dx%d (%zu bytes)\n", width, height, pixel_data.size());

            // Delete old texture if it exists
            if (m_configs.maze_texture_id != 0)
            {
                glDeleteTextures(1, &m_configs.maze_texture_id);
                m_configs.maze_texture_id = 0;
            }

            // Create OpenGL texture
            glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureIdentifier::MAZE));
            glGenTextures(1, &m_configs.maze_texture_id);
            glBindTexture(GL_TEXTURE_2D, m_configs.maze_texture_id);

            // Set texture parameters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Upload texture data
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, pixel_data.data());

            // Generate mipmaps
            glGenerateMipmap(GL_TEXTURE_2D);

            // Check for errors
            if (const GLenum error = glGetError(); error != GL_NO_ERROR)
            {
                SDL_LogError(SDL_LOG_CATEGORY_ERROR, "OpenGL error creating maze texture: 0x%x\n", error);
                return false;
            }

            m_configs.maze_texture_width = width;
            m_configs.maze_texture_height = height;
            m_configs.maze_ready = true;

            // Store the grid for artifact generation and launch async task
            m_maze_future = std::exchange(m_maze_future, std::async(std::launch::async, m_maze_task, m_configs.maze));

            // Enable the download button now that maze data is available
            m_configs.show_download_button = true;

            SDL_Log("Maze texture created successfully: ID=%u, %dx%d\n",
                    m_configs.maze_texture_id, width, height);
            SDL_Log("Async task launched for artifact generation\n");

            // Place maze blocks in the world for visual rendering
            // Parse pixel_data: black pixels (walls) become stone blocks
            // Sample every 'scale' pixels to match logical maze structure

            // Get player position to place maze at player's feet
            const int base_x = static_cast<int>(pos.x);
            const int base_y = static_cast<int>(pos.y);
            const int base_z = static_cast<int>(pos.z);

            // Calculate logical maze dimensions (before scaling)
            const int logical_width = width / scale;
            const int logical_height = height / scale;

            SDL_Log("Placing maze in world: %dx%d logical cells (from %dx%d pixels, scale=%u) at offset (%d, %d, %d)\n",
                    logical_width, logical_height, width, height, scale, base_x, base_y, base_z);

            int blocks_placed = 0;
            const auto wall_height = m_configs.maze.levels();

            // Iterate through logical maze cells by sampling every 'scale' pixels
            // This creates geometry that matches the maze structure, not the upscaled texture
            for (int cell_y = 0; cell_y < logical_height; ++cell_y)
            {
                for (int cell_x = 0; cell_x < logical_width; ++cell_x)
                {
                    // Sample the center of each scaled cell region
                    const int pix_x = cell_x * scale + scale / 2;
                    const int pix_y = cell_y * scale + scale / 2;

                    // Calculate pixel index in the RGBA array
                    const int pixel_index = (pix_y * width + pix_x) * 4;

                    // Read RGBA values
                    const uint8_t r = pixel_data[pixel_index + 0];
                    const uint8_t g = pixel_data[pixel_index + 1];
                    const uint8_t b = pixel_data[pixel_index + 2];
                    // Alpha is pixel_data[pixel_index + 3] but we don't need it

                    // Check if pixel is black (wall) - threshold for near-black
                    const bool is_wall = (r < 50 && g < 50 && b < 50);

                    if (is_wall)
                    {
                        // Place a vertical column of blocks for this wall
                        for (int y = 0; y < wall_height; ++y)
                        {
                            const int world_x = base_x + cell_x;
                            const int world_y = base_y + y;
                            const int world_z = base_z + cell_y;

                            m_world->set_block(world_x, world_y, world_z, get_item());
                            world::record_block(world_x, world_y, world_z, get_item());
                            blocks_placed++;
                        }
                    }
                }
            }

            SDL_Log("Maze blocks placed successfully! %d blocks placed\n", blocks_placed);
            SDL_Log("  Maze covers: X[%d..%d] Y[%d..%d] Z[%d..%d]\n",
                   base_x, base_x + logical_width - 1,
                   base_y, base_y + wall_height - 1,
                   base_z, base_z + logical_height - 1);

            return true;
        }

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create grid\n");
        return false;
    }
    catch (const std::exception& e)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Exception in generate_maze_texture: %s\n", e.what());
        return false;
    }
}

/// Gather player's generated maze artifacts from the async task
/// @return Wavefront .obj data as a string, or empty if not ready
std::string player::artifacts() const noexcept
{
    if (!m_maze_future.valid())
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "artifacts(): Future is not valid. Generate a maze first.\n");
        return "";
    }

    if (const auto status = m_maze_future.wait_for(std::chrono::seconds(0));
        status == std::future_status::ready)
    {
        const auto g = m_maze_future.get();
        if (!g)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "artifacts(): Grid is null\n");
            return "";
        }

        const auto result = g->operations().get_str();
        SDL_Log("artifacts(): Retrieved %zu bytes from future\n", result.size());
        return result;
    }
    else if (status == std::future_status::timeout)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "artifacts(): Maze generation still in progress. Please wait.\n");
    }
    else
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "artifacts(): Future status deferred.\n");
    }

    return "";
}

/// Check if maze generation cooldown has elapsed (non-blocking)
/// @return true if cooldown has elapsed and generation is allowed
bool player::is_maze_generation_ready() const noexcept
{
    const std::uint64_t current_time = SDL_GetTicks();
    const std::uint64_t elapsed = current_time - m_last_maze_generation_time;
    return elapsed >= MAZE_GENERATION_COOLDOWN_MS;
}

/// Get remaining cooldown time in milliseconds
/// @return milliseconds remaining until next generation is allowed (0 if ready)
std::uint64_t player::get_maze_cooldown_remaining_ms() const noexcept
{
    const std::uint64_t current_time = SDL_GetTicks();
    const std::uint64_t elapsed = current_time - m_last_maze_generation_time;

    if (elapsed >= MAZE_GENERATION_COOLDOWN_MS)
    {
        return 0;
    }

    return MAZE_GENERATION_COOLDOWN_MS - elapsed;
}

