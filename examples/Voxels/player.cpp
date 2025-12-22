#include "player.h"

#include <chrono>
#include <cmath>
#include <ranges>
#include <sstream>

#include "command_queue.h"
#include "db.h"
#include "entity.h"
#include "item.h"
#include "matrix.h"
#include "texture.h"
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

namespace
{
    // Helper to convert block/voxel data to Wavefront OBJ format
    std::string blocks_to_wavefront_obj(const std::vector<std::tuple<int, int, int, int>>& blocks) noexcept
    {
        if (blocks.empty())
        {
            return "";
        }

        std::ostringstream result;

        // Write header
        result << "# Voxel World Export\n";
        result << "# Generated from database\n";
        result << "# Block count: " << blocks.size() << "\n\n";

        // Cube vertex offsets (8 vertices per cube)
        static constexpr float cube_vertices[8][3] = {
            {-0.5f, -0.5f, -0.5f}, // 0
            {0.5f, -0.5f, -0.5f}, // 1
            {0.5f, 0.5f, -0.5f}, // 2
            {-0.5f, 0.5f, -0.5f}, // 3
            {-0.5f, -0.5f, 0.5f}, // 4
            {0.5f, -0.5f, 0.5f}, // 5
            {0.5f, 0.5f, 0.5f}, // 6
            {-0.5f, 0.5f, 0.5f} // 7
        };

        // Cube face indices (6 faces, 2 triangles each = 6 vertices per face)
        // Faces: front, back, top, bottom, right, left
        static constexpr int cube_faces[6][6] = {
            {4, 5, 6, 4, 6, 7}, // front  (+Z)
            {1, 0, 3, 1, 3, 2}, // back   (-Z)
            {3, 7, 6, 3, 6, 2}, // top    (+Y)
            {0, 1, 5, 0, 5, 4}, // bottom (-Y)
            {1, 2, 6, 1, 6, 5}, // right  (+X)
            {0, 4, 7, 0, 7, 3} // left   (-X)
        };

        int vertex_count = 0;

        // Generate vertices and faces for each block
        for (const auto& [x, y, z, w] : blocks)
        {
            // Skip air blocks (w == 0)
            if (w == 0)
            {
                continue;
            }

            // Write vertices for this cube
            for (int v = 0; v < 8; ++v)
            {
                float vx = static_cast<float>(x) + cube_vertices[v][0];
                float vy = static_cast<float>(y) + cube_vertices[v][1];
                float vz = static_cast<float>(z) + cube_vertices[v][2];
                result << "v " << vx << " " << vy << " " << vz << "\n";
            }

            // Write faces for this cube (all 6 faces)
            for (int face = 0; face < 6; ++face)
            {
                result << "f";
                for (int i = 0; i < 3; ++i)
                {
                    result << " " << (vertex_count + cube_faces[face][i] + 1);
                }
                result << "\n";

                result << "f";
                for (int i = 3; i < 6; ++i)
                {
                    result << " " << (vertex_count + cube_faces[face][i] + 1);
                }
                result << "\n";
            }

            vertex_count += 8;
        }

        return result.str();
    }
}

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
      , m_is_on_auto_run{false}
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

              mazes::randomizer rng{};
              rng.seed(m_configs.maze.seed());
              if (!a.value()->run(g.get(), std::ref(rng)))
              {
                  return nullptr;
              }

              // Set geometric data
              if (thread_local mazes::wavefront_object_helper woh{}; !woh.run(g.get(), std::ref(rng)))
              {
                  return nullptr;
              }

              // Set bytes
              if (thread_local mazes::pixels pixel_converter; !pixel_converter.run(g.get(), std::ref(rng)))
              {
                  return nullptr;
              }

              return std::move(g);
          }
      }
      , m_grid_factory{std::make_unique<mazes::grid_factory>()}
{
    m_grid_factory->register_creator(m_name, m_maze_task);

    set_category(Entity::PLAYER);

    // Movement key bindings
    assign_key(PlayerAction::MOVE_LEFT, SDL_SCANCODE_A);
    assign_key(PlayerAction::MOVE_RIGHT, SDL_SCANCODE_D);
    assign_key(PlayerAction::MOVE_FORWARD, SDL_SCANCODE_W);
    assign_key(PlayerAction::MOVE_BACKWARD, SDL_SCANCODE_S);
    assign_key(PlayerAction::MOVE_AUTO, SDL_SCANCODE_Q);
    assign_key(PlayerAction::MOVE_UP, SDL_SCANCODE_F);
    assign_key(PlayerAction::MOVE_DOWN, SDL_SCANCODE_LSHIFT);
    assign_key(PlayerAction::JUMP, SDL_SCANCODE_SPACE);
    assign_key(PlayerAction::FLY, SDL_SCANCODE_TAB);
    assign_key(PlayerAction::PLACE_LIGHT, SDL_SCANCODE_LCTRL);
    assign_key(PlayerAction::TAG_SIGN, SDL_SCANCODE_T);
    assign_key(PlayerAction::PLACE_MAZE, SDL_SCANCODE_B);
    assign_key(PlayerAction::PREVIEW_MAZE, SDL_SCANCODE_E);

    m_configs.day_length = DAY_LENGTH;
    m_configs.start_time = DAY_LENGTH / 2 * 1000;
    m_configs.start_ticks = static_cast<int>(SDL_GetTicks());
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
    }
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (event.button.button == SDL_BUTTON_LEFT)
        {
            commands.push(m_action_binding[PlayerAction::DESTROY_BLOCK]);
        }
        else if (event.button.button == SDL_BUTTON_RIGHT)
        {
            commands.push(m_action_binding[PlayerAction::BUILD_BLOCK]);
        }
        else if (event.button.button == SDL_BUTTON_MIDDLE)
        {
            commands.push(m_action_binding[PlayerAction::COPY_BLOCK]);
        }
    }
    if (event.type == SDL_EVENT_MOUSE_MOTION)
    {
        constexpr float mouse_sensitivity = 0.0025f;
        position* player_pos = &this->m_pos;
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

void player::update(float delta_time, mazes::randomizer& rng) noexcept
{
}

void player::draw() const noexcept
{
}

void player::handle_realtime_input(command_queue& commands)
{
    int numKeys = 0;
    const auto* keyState = SDL_GetKeyboardState(&numKeys);

    if (m_is_on_auto_run)
    {
        commands.push(m_action_binding[PlayerAction::MOVE_FORWARD]);
    }

    // Process all realtime action keys
    for (auto& [id, action] : m_key_binding)
    {
        if (is_realtime_action(action))
        {
            // Skip JUMP if not flying
            if (action == PlayerAction::JUMP && !m_is_flying)
            {
                continue;
            }

            if (action == PlayerAction::MOVE_FORWARD && m_is_on_auto_run)
            {
                continue;
            }

            // Check if the key is currently pressed
            if (keyState && id < static_cast<std::uint32_t>(numKeys) && keyState[id])
            {
                commands.push(m_action_binding[action]);
                // Check for disablement
                if (m_is_on_auto_run &&
                    (action == PlayerAction::MOVE_LEFT ||
                        action == PlayerAction::MOVE_RIGHT ||
                        action == PlayerAction::MOVE_BACKWARD))
                {
                    m_is_on_auto_run = false;
                }
            }
        }
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

void player::initialize_actions()
{
    // Movement parameters for smooth interpolation
    constexpr float max_move_speed = 5.0f;
    constexpr float acceleration = 0.2f;

    m_action_binding[PlayerAction::MOVE_BACKWARD].action = derived_action<player>(
        [](player& p, const float dt, mazes::randomizer& rng)
        {
            const float target_vx = -SDL_sinf(p.m_pos.rx) * max_move_speed;
            const float target_vz = SDL_cosf(p.m_pos.rx) * max_move_speed;

            p.m_vel.vx = lerp(p.m_vel.vx, target_vx, acceleration);
            p.m_vel.vz = lerp(p.m_vel.vz, target_vz, acceleration);

            const float dt_seconds = dt / 1000.0f;
            p.m_pos.x += p.m_vel.vx * dt_seconds;
            p.m_pos.z += p.m_vel.vz * dt_seconds;
        });

    m_action_binding[PlayerAction::MOVE_FORWARD].action = derived_action<player>(
        [](player& p, const float dt, mazes::randomizer& rng)
        {
            const float target_vx = SDL_sinf(p.m_pos.rx) * max_move_speed;
            const float target_vz = -SDL_cosf(p.m_pos.rx) * max_move_speed;

            p.m_vel.vx = lerp(p.m_vel.vx, target_vx, acceleration);
            p.m_vel.vz = lerp(p.m_vel.vz, target_vz, acceleration);

            const float dt_seconds = dt / 1000.0f;
            p.m_pos.x += p.m_vel.vx * dt_seconds;
            p.m_pos.z += p.m_vel.vz * dt_seconds;
        });

    m_action_binding[PlayerAction::MOVE_LEFT].action = derived_action<player>(
        [](player& p, const float dt, mazes::randomizer& rng)
        {
            const float target_vx = -SDL_cosf(p.m_pos.rx) * max_move_speed;
            const float target_vz = -SDL_sinf(p.m_pos.rx) * max_move_speed;

            p.m_vel.vx = lerp(p.m_vel.vx, target_vx, acceleration);
            p.m_vel.vz = lerp(p.m_vel.vz, target_vz, acceleration);

            const float dt_seconds = dt / 1000.0f;
            p.m_pos.x += p.m_vel.vx * dt_seconds;
            p.m_pos.z += p.m_vel.vz * dt_seconds;
        });

    m_action_binding[PlayerAction::MOVE_RIGHT].action = derived_action<player>(
        [](player& p, const float dt, mazes::randomizer& rng)
        {
            const float target_vx = SDL_cosf(p.m_pos.rx) * max_move_speed;
            const float target_vz = SDL_sinf(p.m_pos.rx) * max_move_speed;

            p.m_vel.vx = lerp(p.m_vel.vx, target_vx, acceleration);
            p.m_vel.vz = lerp(p.m_vel.vz, target_vz, acceleration);

            const float dt_seconds = dt / 1000.0f;
            p.m_pos.x += p.m_vel.vx * dt_seconds;
            p.m_pos.z += p.m_vel.vz * dt_seconds;
        });

    m_action_binding[PlayerAction::MOVE_AUTO].action = derived_action<player>(
        [](player& p, const float dt, mazes::randomizer& rng)
        {
            p.m_is_on_auto_run = !p.m_is_on_auto_run;
        });

    m_action_binding[PlayerAction::JUMP].action = derived_action<player>(
        [](player& p, float dt, mazes::randomizer& rng)
        {
            if (p.m_is_flying)
            {
                // In flying mode, move up
                constexpr float flySpeed = 0.15f;
                p.m_vel.vy = flySpeed;
            }
            else if (p.m_on_ground)
            {
                // Normal jump when on ground
                constexpr float jumpVelocity = 8.0f;
                p.m_vel.vy = jumpVelocity;
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
                constexpr float flySpeed = 4.85f;
                p.m_vel.vy = -flySpeed;
            }
        });

    m_action_binding[PlayerAction::MOVE_UP].action = derived_action<player>(
        [](player& p, float dt, mazes::randomizer& rng)
        {
            if (p.m_is_flying)
            {
                constexpr float flySpeed = 4.85f;
                p.m_vel.vy = flySpeed;
            }
        });

    m_action_binding[PlayerAction::FLY].action = derived_action<player>(
        [](player& p, float dt, mazes::randomizer& rng)
        {
            p.m_is_flying = !p.m_is_flying;
            if (p.m_is_flying)
            {
                p.m_vel.vy = 0.0f;
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

    m_action_binding[PlayerAction::COPY_BLOCK].action = derived_action<player>(
        [this](const player& p, float dt, mazes::randomizer& rng)
        {
            if (p.m_world)
            {
                on_middle_click();
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
            constexpr auto PREVIEW_COOLDOWN_MS = 250;
            static auto last_preview_time = SDL_GetTicks();
            const auto current_time = SDL_GetTicks();

            if (const auto time_since_last_preview = current_time - last_preview_time;
                time_since_last_preview > PREVIEW_COOLDOWN_MS && p.m_configs.preview_enabled && p.m_world)
            {
                last_preview_time = current_time;

                // Generate maze grid
                if (auto g = p.m_grid_factory->create(p.get_name(), std::cref(p.m_configs.maze));
                    g.has_value())
                {
                    // Move grid ownership
                    auto grid_ptr = std::move(g.value());
                    if (p.m_world->update_preview(grid_ptr.get()))
                    {
                        // Get pixel data from the generated maze
                        const auto pixel_data = grid_ptr->operations().get_pixels();
                        auto [rows, columns, _] = grid_ptr->operations().get_dimensions();

                        // Calculate scale
                        constexpr unsigned int MIN_SCALE = 1;
                        constexpr unsigned int MAX_SCALE = 10;
                        const auto calculated_scale = static_cast<unsigned int>(SDL_sqrtf(rows * columns));
                        const auto scale = std::clamp(calculated_scale, MIN_SCALE, MAX_SCALE);

                        const auto ascii_height = rows * 2 + 1;
                        const int height = static_cast<int>(ascii_height * scale);
                        const int width = static_cast<int>(pixel_data.size() / (height * 4));

                        // Queue async block placement
                        p.m_world->finalize_buildings(
                            pixel_data,
                            width,
                            height,
                            scale,
                            p.m_configs.maze.levels(),
                            p.get_item()
                        );

                        p.m_configs.download_ready = true;
                        p.m_last_maze_generation_time = SDL_GetTicks();
                    }
                    else
                    {
                        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to generate preview texture\n");
                    }
                }
            }
        });

    m_action_binding[PlayerAction::PLACE_MAZE].action = derived_action<player>(
        [](player& p, const float dt, mazes::randomizer& rng)
        {
            if (!p.m_world)
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "No world available for maze building\n");
                return;
            }

            // Check if we have a valid crosshair target
            if (!p.m_world->m_projected_plane.has_valid_target)
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "No valid target block. Aim at a block face first.\n");
                return;
            }

            // Commit the latest preview to the main world database
            // This moves blocks from preview_blocks table to the main block table
            p.m_world->commit_preview_to_world();

            p.m_configs.download_ready = true;
            SDL_Log("Maze committed to world - press 'E' to generate a new preview\n");
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
    const position* s = &this->m_pos;
    int hx, hy, hz;
    if (const int hw = m_world->hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        hy > 0 && hy < item::TOTAL_BLOCKS && item::is_destructable(hw))
    {
        m_world->toggle_light(hx, hy, hz);
    }
}

void player::on_left_click() const noexcept
{
    const position* s = &this->m_pos;
    int hx, hy, hz;
    if (const auto hw = m_world->hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        hy > 0 && hy < 256 && item::is_destructable(hw))
    {
        m_world->set_block(hx, hy, hz, 0);

        if (item::is_plant(m_world->get_block(hx, hy + 1, hz)))
        {
            m_world->set_block(hx, hy + 1, hz, 0);
        }
    }
}

void player::on_right_click() const noexcept
{
    const position* s = &this->m_pos;
    int hx, hy, hz;
    if (const int hw = m_world->hit_test(1, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        hy > 0 && hy < item::TOTAL_BLOCKS && item::is_obstacle(hw))
    {
        if (!world::player_intersects_block(2, s->x, s->y, s->z, hx, hy, hz))
        {
            m_world->set_block(hx, hy, hz, get_item());
        }
    }
}

void player::on_middle_click() noexcept
{
    const position* s = &this->m_pos;
    int hx, hy, hz;
    const int hw = m_world->hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    for (int i = 0; i < item::items.size(); i++)
    {
        if (item::items.at(i) == hw)
        {
            this->m_item_index = i;
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

/// Gather player's voxel world artifacts from the database
/// @return Wavefront .obj data as a string, or empty if not ready
std::string player::artifacts() const noexcept
{
    // Check if database is enabled
    if (!get_db_enabled())
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Database not enabled for artifacts export\n");
        return "";
    }

    // Calculate player's chunk coordinates
    const int player_chunk_p = m_world->chunked(m_pos.x);
    const int player_chunk_q = m_world->chunked(m_pos.z);
    // Query blocks from nearby chunks - increased radius for better coverage
    // Radius of 4 chunks = 9x9 chunk area (~2304 blocks if fully populated)
    constexpr int chunk_radius = 4;
    const auto blocks = db_query_blocks_near_chunks(player_chunk_p, player_chunk_q, chunk_radius);

    if (blocks.empty())
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "No blocks found in database for export\n");
        return "";
    }

    // Convert blocks to Wavefront OBJ format
    return blocks_to_wavefront_obj(blocks);
}

