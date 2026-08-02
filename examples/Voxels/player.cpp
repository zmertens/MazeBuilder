#include "player.h"

#include <chrono>
#include <cmath>
#include <ranges>
#include <sstream>

#include "db.h"
#include "geometries.h"
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
#include <MazeBuilder/randomizer.h>
#include <MazeBuilder/string_utils.h>

namespace
{
    // Helper to convert block/voxel data to Wavefront OBJ format
    std::string blocks_to_wavefront_obj(const std::vector<std::tuple<int, int, int, int>> &blocks) noexcept
    {
        if (blocks.empty())
        {
            return "";
        }

        const auto start_time = SDL_GetTicks();

        std::ostringstream result;
        // Pre-allocate buffer to reduce reallocations (estimate ~200 bytes per block)
        result.str().reserve(blocks.size() * 200);

        // Write header
        result << "# Voxel World Export\n";
        result << "# Generated from database\n";
        result << "# Block count: " << blocks.size() << "\n\n";

        // Cube vertex offsets (8 vertices per cube)
        static constexpr float cube_vertices[8][3] = {
            {-0.5f, -0.5f, -0.5f}, // 0
            {0.5f, -0.5f, -0.5f},  // 1
            {0.5f, 0.5f, -0.5f},   // 2
            {-0.5f, 0.5f, -0.5f},  // 3
            {-0.5f, -0.5f, 0.5f},  // 4
            {0.5f, -0.5f, 0.5f},   // 5
            {0.5f, 0.5f, 0.5f},    // 6
            {-0.5f, 0.5f, 0.5f}    // 7
        };

        // Cube face indices (6 faces, 2 triangles each = 6 vertices per face)
        // Faces: front, back, top, bottom, right, left
        static constexpr int cube_faces[6][6] = {
            {4, 5, 6, 4, 6, 7}, // front  (+Z)
            {1, 0, 3, 1, 3, 2}, // back   (-Z)
            {3, 7, 6, 3, 6, 2}, // top    (+Y)
            {0, 1, 5, 0, 5, 4}, // bottom (-Y)
            {1, 2, 6, 1, 6, 5}, // right  (+X)
            {0, 4, 7, 0, 7, 3}  // left   (-X)
        };

        int vertex_count = 0;
        int processed_blocks = 0;

        // Generate vertices and faces for each block
        for (const auto &[x, y, z, w] : blocks)
        {
            // Skip air blocks (w == 0)
            if (w == 0)
            {
                continue;
            }

            // Progress logging for large exports (every 10,000 blocks)
            if (++processed_blocks % 10000 == 0)
            {
                SDL_Log("OBJ export progress: %d / %zu blocks\n", processed_blocks, blocks.size());
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
                const int base = vertex_count + 1;
                result << "f " << base + cube_faces[face][0]
                       << " " << base + cube_faces[face][1]
                       << " " << base + cube_faces[face][2] << "\n"
                       << "f " << base + cube_faces[face][3]
                       << " " << base + cube_faces[face][4]
                       << " " << base + cube_faces[face][5] << "\n";
            }

            vertex_count += 8;
        }

        const auto elapsed = SDL_GetTicks() - start_time;
        SDL_Log("blocks_to_wavefront_obj: Generated OBJ with %d blocks in %u ms\n", processed_blocks, elapsed);

        return result.str();
    }
}

constexpr auto DAY_LENGTH = 600;
constexpr auto DEFAULT_FOV = 65.0f;
constexpr auto DEFAULT_ORTHO = 0u;
constexpr auto ORTHO_ENABLED_VAL = 64;
constexpr auto SCROLL_THRESHOLD = 0.1f;
constexpr auto ZOOM_FOV = 15.f;
constexpr auto ORTHO_MIN_SCALE = 8;
constexpr auto ORTHO_MAX_SCALE = 96;

player::player()
    : scene_node{}, m_is_active{true}, m_on_ground{false}, m_is_flying{false}, m_is_on_auto_run{false}, m_name{"zm"}, m_buffer{}, m_item_index{0}, m_world{nullptr}, m_maze_task{geometries::generate_maze_preview}
{
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
    assign_key(PlayerAction::COPY_BLOCK, SDL_SCANCODE_C);
    assign_key(PlayerAction::CHANGE_PERSPECTIVE, SDL_SCANCODE_O);
    assign_key(PlayerAction::ZOOM_IN_ISO_VIEW, SDL_SCANCODE_EQUALS);
    assign_key(PlayerAction::ZOOM_OUT_ISO_VIEW, SDL_SCANCODE_MINUS);

    m_configs.day_length = DAY_LENGTH;
    m_configs.start_time = DAY_LENGTH / 2 * 1000;
    m_configs.start_ticks = SDL_GetTicks();
    m_configs.fov = DEFAULT_FOV;
    m_configs.ortho = static_cast<int>(OrthoViewMode::FIXED_INT_FOR_ISO_VIEW);
    m_configs.invert_mouse = false;
    m_configs.tag = "put maze here";
    m_configs.maze
        .algo_id(mazes::algo::DFS)
        .rows(10)
        .columns(10)
        .levels(3)
        .seed(42u);

    initialize_actions();

    // Set category for all player actions
    for (auto &[_, category] : m_action_binding | std::views::values)
    {
        category = Entity::PICKUP;
    }
}

void player::handle_event(const SDL_Event &event, command_queue &commands) noexcept
{
    if (event.type == SDL_EVENT_QUIT)
    {
        m_is_active = false;
    }
    if (event.type == SDL_EVENT_MOUSE_WHEEL)
    {
        // Note: items array has 58 valid items (0-57), though array size is 64
        constexpr std::int32_t MAX_ITEM_INDEX = 57;

        if (event.wheel.y > SCROLL_THRESHOLD)
        {
            // Scroll up (backward through items)
            if (m_item_index > 0)
            {
                m_item_index--;
            }
            else
            {
                m_item_index = MAX_ITEM_INDEX; // Wrap to last valid item
            }
        }
        else if (event.wheel.y < -SCROLL_THRESHOLD)
        {
            // Scroll down (forward through items)
            if (m_item_index < MAX_ITEM_INDEX)
            {
                m_item_index++;
            }
            else
            {
                m_item_index = 0; // Wrap to first item
            }
        }

        // The cached maze preview is tied to the selected build item.
        // Invalidate it so B cannot reuse stale preview state after a hotbar change.
        if (m_world)
        {
            m_world->invalidate_preview();
        }
        m_auto_preview_pending = true;
    }
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        // Handle CAD feature keys first (H, G, O, R)
        switch (event.key.scancode)
        {
        case SDL_SCANCODE_H:
            toggle_hover_display();
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Hover Display: %s",
                        m_configs.show_hover_info ? "ON" : "OFF");
            return;
        case SDL_SCANCODE_G:
            toggle_grid();
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Grid Overlay: %s",
                        m_configs.show_grid ? "ON" : "OFF");
            return;
        case SDL_SCANCODE_O:
            cycle_ortho_view();
            {
                const char *view_name = "Perspective";
                switch (m_configs.ortho_view_mode)
                    switch (m_configs.ortho_view_mode)
                    {
                    case OrthoViewMode::ISOMETRIC:
                        view_name = "Isometric View";
                        break;
                    default:
                        break;
                    }
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "View Mode: %s", view_name);
            }
            return;
        case SDL_SCANCODE_R:
            activate_measurement_tool();
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Measurement Tool: %s",
                        m_configs.active_cad_tool == CADTool::MEASURE_DISTANCE ? "ACTIVE" : "OFF");
            return;
        case SDL_SCANCODE_EQUALS:
        case SDL_SCANCODE_KP_PLUS:
            if (m_configs.ortho_view_mode == OrthoViewMode::ISOMETRIC)
            {
                m_configs.ortho = SDL_max(ORTHO_MIN_SCALE, m_configs.ortho - 2);
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Isometric Zoom: %d", m_configs.ortho);
                return;
            }
            break;
        case SDL_SCANCODE_MINUS:
        case SDL_SCANCODE_KP_MINUS:
            if (m_configs.ortho_view_mode == OrthoViewMode::ISOMETRIC)
            {
                m_configs.ortho = SDL_min(ORTHO_MAX_SCALE, m_configs.ortho + 2);
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Isometric Zoom: %d", m_configs.ortho);
                return;
            }
            break;
        default:
            break;
        }

        // Handle regular key bindings
        if (const auto found = m_key_binding.find(event.key.scancode);
            found != m_key_binding.cend() && !is_realtime_action(found->second))
        {
            const auto binding = m_action_binding.find(found->second);
            if (binding == m_action_binding.cend() || !binding->second.action)
            {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Ignoring unmapped player action %d\n",
                            static_cast<int>(found->second));
                return;
            }

            if (found->second == PlayerAction::JUMP)
            {
                // Only allow jumping when on ground
                if (m_on_ground)
                {
                    commands.push(binding->second);
                }
                return;
            }
            commands.push(binding->second);
        }
    }
    if (event.type == SDL_EVENT_KEY_UP)
    {
    }
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (event.button.button == SDL_BUTTON_LEFT)
        {
            // If measurement tool is active, record point instead of destroying
            if (m_configs.active_cad_tool == CADTool::MEASURE_DISTANCE && m_world && m_world->m_projected_plane.has_valid_target)
            {
                record_measurement_point(m_world->m_projected_plane.target_x,
                                         m_world->m_projected_plane.target_y,
                                         m_world->m_projected_plane.target_z);
                return;
            }
            if (const auto binding = m_action_binding.find(PlayerAction::DESTROY_BLOCK);
                binding != m_action_binding.cend() && binding->second.action)
            {
                commands.push(binding->second);
            }
        }
        else if (event.button.button == SDL_BUTTON_RIGHT)
        {
            if (const auto binding = m_action_binding.find(PlayerAction::BUILD_BLOCK);
                binding != m_action_binding.cend() && binding->second.action)
            {
                commands.push(binding->second);
            }
        }
        else if (event.button.button == SDL_BUTTON_MIDDLE)
        {
            if (const auto binding = m_action_binding.find(PlayerAction::COPY_BLOCK);
                binding != m_action_binding.cend() && binding->second.action)
            {
                commands.push(binding->second);
            }
        }
    }
    if (event.type == SDL_EVENT_MOUSE_MOTION)
    {
        constexpr float mouse_sensitivity = 0.0025f;
        position *player_pos = &this->m_pos;
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
            player_pos->rx += matrix::to_radians(360.0f);
        }
        if (player_pos->rx >= matrix::to_radians(360.0f))
        {
            player_pos->rx -= matrix::to_radians(360.0f);
        }
        player_pos->ry = SDL_max(player_pos->ry, -matrix::to_radians(90.0f));
        player_pos->ry = SDL_min(player_pos->ry, matrix::to_radians(90.0f));
    }
}

void player::update(float delta_time, mazes::randomizer &rng) noexcept
{
    // Auto-generate the first preview as soon as a world is available.
    if (m_world && m_auto_preview_pending)
    {
        m_auto_preview_pending = false;
        request_preview_generation();
    }
    process_preview_generation();
}

void player::draw() const noexcept
{
}

void player::handle_realtime_input(command_queue &commands)
{
    int numKeys = 0;
    const auto *keyState = SDL_GetKeyboardState(&numKeys);

    if (m_is_on_auto_run)
    {
        if (const auto binding = m_action_binding.find(PlayerAction::MOVE_FORWARD);
            binding != m_action_binding.cend() && binding->second.action)
        {
            commands.push(binding->second);
        }
    }

    // Process all realtime action keys
    for (auto &[id, action] : m_key_binding)
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
                if (const auto binding = m_action_binding.find(action);
                    binding != m_action_binding.cend() && binding->second.action)
                {
                    commands.push(binding->second);
                }
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
    std::erase_if(m_key_binding, [action](const auto &kv)
                  { return kv.second == action; });
    m_key_binding.insert_or_assign(key, action);
}

[[nodiscard]] std::uint32_t player::get_assigned_key(const PlayerAction action) const
{
    const auto it = std::ranges::find_if(m_key_binding,
                                         [action](const auto &kv)
                                         { return kv.second == action; });
    return it != m_key_binding.end() ? it->first : static_cast<std::uint32_t>(SDL_SCANCODE_UNKNOWN);
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

void player::set_name(const std::string &name) noexcept
{
    m_name = name;
}

void player::set_world(world *w) noexcept
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
        [](player &p, const float dt, mazes::randomizer &rng)
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
        [](player &p, const float dt, mazes::randomizer &rng)
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
        [](player &p, const float dt, mazes::randomizer &rng)
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
        [](player &p, const float dt, mazes::randomizer &rng)
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
        [](player &p, const float dt, mazes::randomizer &rng)
        {
            p.m_is_on_auto_run = !p.m_is_on_auto_run;
        });

    m_action_binding[PlayerAction::JUMP].action = derived_action<player>(
        [](player &p, float dt, mazes::randomizer &rng)
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
        [](player &p, float dt, mazes::randomizer &rng)
        {
            if (p.m_world)
            {
                p.on_tag_sign();
            }
        });

    m_action_binding[PlayerAction::MOVE_DOWN].action = derived_action<player>(
        [](player &p, float dt, mazes::randomizer &rng)
        {
            if (p.m_is_flying)
            {
                constexpr float flySpeed = 4.85f;
                p.m_vel.vy = -flySpeed;
            }
        });

    m_action_binding[PlayerAction::MOVE_UP].action = derived_action<player>(
        [](player &p, float dt, mazes::randomizer &rng)
        {
            if (p.m_is_flying)
            {
                constexpr float flySpeed = 4.85f;
                p.m_vel.vy = flySpeed;
            }
        });

    m_action_binding[PlayerAction::FLY].action = derived_action<player>(
        [](player &p, float dt, mazes::randomizer &rng)
        {
            p.m_is_flying = !p.m_is_flying;
            if (p.m_is_flying)
            {
                p.m_vel.vy = 0.0f;
            }
        });

    m_action_binding[PlayerAction::BUILD_BLOCK].action = derived_action<player>(
        [](const player &p, float dt, mazes::randomizer &rng)
        {
            if (p.m_world)
            {
                p.on_right_click();
            }
        });

    m_action_binding[PlayerAction::COPY_BLOCK].action = derived_action<player>(
        [](player &p, float dt, mazes::randomizer &rng)
        {
            if (p.m_world)
            {
                p.on_middle_click();
            }
        });

    m_action_binding[PlayerAction::DESTROY_BLOCK].action = derived_action<player>(
        [](const player &p, float dt, mazes::randomizer &rng)
        {
            if (p.m_world)
            {
                p.on_left_click();
            }
        });

    m_action_binding[PlayerAction::PLACE_LIGHT].action = derived_action<player>(
        [](const player &p, float dt, mazes::randomizer &rng)
        {
            if (p.m_world)
            {
                p.on_light();
            }
        });

    m_action_binding[PlayerAction::PREVIEW_MAZE].action = derived_action<player>(
        [](player &p, const float dt, mazes::randomizer &rng)
        {
            constexpr auto PREVIEW_COOLDOWN_MS = 250;
            const auto current_time = SDL_GetTicks();

            if (const auto time_since_last_preview_request = current_time - p.m_last_preview_request_time;
                time_since_last_preview_request > PREVIEW_COOLDOWN_MS)
            {
                if (p.request_preview_generation())
                {
                    p.m_last_preview_request_time = current_time;
                }
            }
        });

    m_action_binding[PlayerAction::PLACE_MAZE].action = derived_action<player>(
        [](player &p, const float dt, mazes::randomizer &rng)
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

            p.m_world->commit_preview_to_world(p.get_item());
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
    case PlayerAction::PREVIEW_MAZE: // polled every frame; cooldown in action lambda
        return true;
    case PlayerAction::CHANGE_PERSPECTIVE:
        [[fallthrough]];
    case PlayerAction::ZOOM_IN_ISO_VIEW:
        [[fallthrough]];
    case PlayerAction::ZOOM_OUT_ISO_VIEW:
        [[fallthrough]];
    default:
        return false;
    }
}

void player::on_light() const noexcept
{
    const position *s = &this->m_pos;
    int hx, hy, hz;
    if (const int hw = m_world->hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        hy > 0 && hy < item::TOTAL_BLOCKS && item::is_destructable(hw))
    {
        m_world->toggle_light(hx, hy, hz);
    }
}

void player::on_left_click() const noexcept
{
    const position *s = &this->m_pos;
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
    const position *s = &this->m_pos;
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
    const position *s = &this->m_pos;
    int hx, hy, hz;
    const int hw = m_world->hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    if (const auto it = std::ranges::find(item::items, hw); it != item::items.end())
        m_item_index = static_cast<std::int32_t>(it - item::items.begin());
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

bool player::preview_generation_in_progress() const noexcept
{
    if (!m_preview_future.valid())
    {
        return false;
    }

    return m_preview_future.wait_for(std::chrono::milliseconds{0}) != std::future_status::ready;
}

bool player::request_preview_generation() noexcept
{
    if (!m_world)
    {
        return false;
    }

    if (preview_generation_in_progress())
    {
        return false;
    }

    try
    {
        const auto config = m_configs.maze;
        auto maze_task = m_maze_task;

#if defined(__EMSCRIPTEN__)
        // Web builds share the runtime_app singleton with rendering, so run preview
        // generation synchronously to avoid racing the GL/event loop.
        m_pending_preview = maze_task(config);
#else
        m_preview_future = std::async(std::launch::async, [maze_task = std::move(maze_task), config]() mutable
                                      { return maze_task(config); });
#endif
        return true;
    }
    catch (const std::exception &)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to schedule maze preview generation\n");
        return false;
    }
}

void player::process_preview_generation() noexcept
{
    if (!m_world)
    {
        return;
    }

#if defined(__EMSCRIPTEN__)
    if (!m_pending_preview.has_value())
    {
        return;
    }

    auto preview = std::move(m_pending_preview);
    m_pending_preview.reset();
#else
    if (!m_preview_future.valid())
    {
        return;
    }

    if (m_preview_future.wait_for(std::chrono::milliseconds{0}) != std::future_status::ready)
    {
        return;
    }

    auto preview = m_preview_future.get();
#endif
    if (!preview.has_value())
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Maze preview generation failed\n");
        return;
    }

    if (!m_world->update_preview(preview->pixel_data, preview->width, preview->height))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to upload maze preview texture\n");
        return;
    }

    m_world->finalize_buildings(
        std::move(preview->pixel_data),
        preview->width,
        preview->height,
        preview->scale,
        m_configs.maze.levels(),
        get_item());

    m_last_preview_generation_time = SDL_GetTicks();
}

/// Gather player's voxel world artifacts from the database
/// @return Wavefront .obj data as a string, or empty if not ready
std::string player::artifacts() const noexcept
{
    // Check if database is enabled
    if (!db_is_enabled())
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Database not enabled for artifacts export\n");
        return "";
    }

    // Calculate player's chunk coordinates
    const int player_chunk_p = world::chunked(m_pos.x);
    const int player_chunk_q = world::chunked(m_pos.z);
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

bool player::is_download_ready() const noexcept
{
    return m_configs.artifacts_ready;
}

// ============================================================================
// Async Artifact Export
// ============================================================================

void player::start_async_artifact_export() noexcept
{
    // Don't start a new export if one is already running (future is valid and not ready)
    if (m_artifact_export_future.valid() &&
        m_artifact_export_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Artifact export already in progress\n");
        return;
    }

    SDL_Log("Starting async artifact export...\n");

    if (!db_is_enabled())
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Database not enabled for artifacts export\n");
        // Set future to a ready empty result so callers don't spin forever
        std::promise<std::string> p;
        p.set_value("");
        m_artifact_export_future = p.get_future();
        return;
    }

    // Query blocks on the calling (main) thread.
    // SQLite is not safe to call from std::async worker threads in Emscripten
    // (SQLITE_THREADSAFE=0 builds) because sqlite3_step runs outside load_mtx
    // while the db_worker can concurrently execute "commit; begin;", causing a
    // data race that silently returns 0 rows.
    const int player_chunk_p = world::chunked(m_pos.x);
    const int player_chunk_q = world::chunked(m_pos.z);

    SDL_Log("Async export: Querying database for blocks...\n");
    constexpr int chunk_radius = 4;
    auto blocks = db_query_blocks_near_chunks(player_chunk_p, player_chunk_q, chunk_radius);

    if (blocks.empty())
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "No blocks found in database for export\n");
        std::promise<std::string> p;
        p.set_value("");
        m_artifact_export_future = p.get_future();
        return;
    }

    m_cached_artifact_result.clear();

    // Offload only the CPU-bound OBJ conversion to a background thread
    m_artifact_export_future = std::async(std::launch::async, [blocks = std::move(blocks)]() -> std::string
                                          {
        SDL_Log("Async export: Converting %zu blocks to OBJ format...\n", blocks.size());
        return blocks_to_wavefront_obj(blocks); });
}

bool player::is_artifact_export_ready() const noexcept
{
    return m_artifact_export_future.valid() &&
           m_artifact_export_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

std::string player::get_artifact_export_result() noexcept
{
    if (!is_artifact_export_ready())
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Artifact export not ready yet\n");
        return "";
    }

    // Cache the result so we can return it multiple times
    if (m_cached_artifact_result.empty())
    {
        m_cached_artifact_result = m_artifact_export_future.get();
        SDL_Log("Async export complete: %zu bytes\n", m_cached_artifact_result.size());
    }

    return m_cached_artifact_result;
}

// ============================================================================
// CAD Helper Functions (Tier 1)
// ============================================================================

const char *player::get_block_name(const int block_type) noexcept
{
    switch (block_type)
    {
    case EMPTY:
        return "Empty";
    case GRASS:
        return "Grass";
    case SAND:
        return "Sand";
    case STONE:
        return "Stone";
    case BRICK:
        return "Brick";
    case WOOD:
        return "Wood";
    case CEMENT:
        return "Cement";
    case DIRT:
        return "Dirt";
    case PLANK:
        return "Plank";
    case SNOW:
        return "Snow";
    case GLASS:
        return "Glass";
    case COBBLE:
        return "Cobblestone";
    case LIGHT_STONE:
        return "Light Stone";
    case DARK_STONE:
        return "Dark Stone";
    case CHEST:
        return "Chest";
    case LEAVES:
        return "Leaves";
    case CLOUD:
        return "Cloud";
    case TALL_GRASS:
        return "Tall Grass";
    case YELLOW_FLOWER:
        return "Yellow Flower";
    case RED_FLOWER:
        return "Red Flower";
    case PURPLE_FLOWER:
        return "Purple Flower";
    case SUN_FLOWER:
        return "Sun Flower";
    case WHITE_FLOWER:
        return "White Flower";
    case BLUE_FLOWER:
        return "Blue Flower";
    case SDL_LOGO:
        return "SDL Logo";
    case SFML_LOGO:
        return "SFML Logo";
    case CACTUS_1:
        return "Cactus 1";
    case CACTUS_2:
        return "Cactus 2";
    default:
        if (block_type >= COLOR_00 && block_type <= COLOR_31)
        {
            static char color_name[32];
            SDL_snprintf(color_name, sizeof(color_name), "Color %02d", block_type - COLOR_00);
            return color_name;
        }
        return "Unknown";
    }
}

const char *player::get_face_name(const int face) noexcept
{
    switch (face)
    {
    case 0:
        return "Left";
    case 1:
        return "Right";
    case 2:
        return "Top";
    case 3:
        return "Bottom";
    case 4:
        return "Front";
    case 5:
        return "Back";
    default:
        return "Unknown";
    }
}

void player::cycle_ortho_view() noexcept
{
    const auto current = static_cast<int>(m_configs.ortho_view_mode);
    const auto next = (current + 1) % 2; // 2 total view modes
    m_configs.ortho_view_mode = static_cast<OrthoViewMode>(next);

    // Update legacy ortho value based on view mode
    switch (m_configs.ortho_view_mode)
    {
    case OrthoViewMode::PERSPECTIVE:
        m_configs.ortho = 0;
        break;
    case OrthoViewMode::ISOMETRIC:
        m_configs.ortho = static_cast<int>(OrthoViewMode::FIXED_INT_FOR_ISO_VIEW);
        break;
    }
}

void player::toggle_grid() noexcept
{
    m_configs.show_grid = !m_configs.show_grid;
}

void player::toggle_hover_display() noexcept
{
    m_configs.show_hover_info = !m_configs.show_hover_info;
}

void player::activate_measurement_tool() noexcept
{
    if (m_configs.active_cad_tool == CADTool::MEASURE_DISTANCE)
    {
        // Deactivate if already active
        m_configs.active_cad_tool = CADTool::NONE;
        clear_measurement();
    }
    else
    {
        // Activate measurement tool
        m_configs.active_cad_tool = CADTool::MEASURE_DISTANCE;
        clear_measurement();
    }
}

void player::record_measurement_point(const int x, const int y, const int z) noexcept
{
    if (!m_measure_point1.valid)
    {
        // Record first point
        m_measure_point1.x = x;
        m_measure_point1.y = y;
        m_measure_point1.z = z;
        m_measure_point1.valid = true;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Measurement point 1: (%d, %d, %d)", x, y, z);
    }
    else if (!m_measure_point2.valid)
    {
        // Record second point and calculate distance
        m_measure_point2.x = x;
        m_measure_point2.y = y;
        m_measure_point2.z = z;
        m_measure_point2.valid = true;

        const int dx = m_measure_point2.x - m_measure_point1.x;
        const int dy = m_measure_point2.y - m_measure_point1.y;
        const int dz = m_measure_point2.z - m_measure_point1.z;
        const float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy + dz * dz));

        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Measurement point 2: (%d, %d, %d) - Distance: %.2f blocks",
                    x, y, z, distance);
    }
    else
    {
        // Already have two points, start over
        clear_measurement();
        record_measurement_point(x, y, z);
    }
}

void player::clear_measurement() noexcept
{
    m_measure_point1.valid = false;
    m_measure_point2.valid = false;
}
