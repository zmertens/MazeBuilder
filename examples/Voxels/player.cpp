#include "player.h"

#include <ranges>

#include "command_queue.h"
#include "entity.h"
#include "item.h"
#include "matrix.h"
#include "world.h"

#include <SDL3/SDL.h>

#include <MazeBuilder/string_utils.h>

#define SCROLL_THRESHOLD 0.1

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
{
    set_category(Entity::PLAYER);

    // Movement key bindings
    m_key_binding[SDL_SCANCODE_A] = PlayerAction::MOVE_LEFT;
    m_key_binding[SDL_SCANCODE_D] = PlayerAction::MOVE_RIGHT;
    m_key_binding[SDL_SCANCODE_W] = PlayerAction::MOVE_FORWARD;
    m_key_binding[SDL_SCANCODE_S] = PlayerAction::MOVE_BACKWARD;
    m_key_binding[SDL_SCANCODE_T] = PlayerAction::TAG_SIGN;
    m_key_binding[SDL_SCANCODE_SPACE] = PlayerAction::JUMP;
    m_key_binding[SDL_SCANCODE_LSHIFT] = PlayerAction::MOVE_DOWN;
    m_key_binding[SDL_SCANCODE_RSHIFT] = PlayerAction::MOVE_UP;
    m_key_binding[SDL_SCANCODE_TAB] = PlayerAction::FLY;

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
            on_middle_click();
        }
    }
    if (event.type == SDL_EVENT_MOUSE_MOTION)
    {
        constexpr float mouse_sensitivity = 0.0025f;
        position* player_pos = &this->pos;
        player_pos->rx += event.motion.xrel * mouse_sensitivity;
        static constexpr auto INVERT_MOUSE = false;
        if (INVERT_MOUSE)
        {
            player_pos->ry += event.motion.yrel * mouse_sensitivity;
        }
        player_pos->ry -= event.motion.yrel * mouse_sensitivity;

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

void player::set_world(world* w) noexcept
{
    m_world = w;
}

std::string_view player::get_local_time() const noexcept
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
    int minute = static_cast<int>((total_hours - hour) * 60.0f);

    // Convert to 12-hour format
    const std::string_view am_pm = hour < 12 ? "am" : "pm";
    hour = hour % 12;
    hour = hour ? hour : 12; // Convert 0 to 12 for midnight/noon

    return mazes::string_utils::format("{}:{:02d}{}", hour, minute, am_pm);
}


void player::initialize_actions()
{
    // Movement parameters for smooth interpolation
    constexpr float max_move_speed = 5.0f;
    constexpr float acceleration = 0.2f;

    m_action_binding[PlayerAction::MOVE_BACKWARD].action = derived_action<player>(
        [](player& p, const float dt)
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
        [](player& p, const float dt)
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
        [](player& p, const float dt)
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
        [](player& p, const float dt)
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
        [](player& p, float dt)
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
        [this](player& p, float dt)
        {
            if (m_world)
            {
                on_tag_sign();
            }
        });

    m_action_binding[PlayerAction::MOVE_DOWN].action = derived_action<player>(
        [](player& p, float dt)
        {
            if (p.m_is_flying)
            {
                // In flying mode, move down
                constexpr float flySpeed = 4.85f;
                p.vel.vy = -flySpeed;
            }
        });

    m_action_binding[PlayerAction::MOVE_UP].action = derived_action<player>(
        [](player& p, float dt)
        {
            if (p.m_is_flying)
            {
                // In flying mode, move down
                constexpr float flySpeed = 4.85f;
                p.vel.vy = flySpeed;
            }
        });

    m_action_binding[PlayerAction::FLY].action = derived_action<player>(
        [](player& p, float dt)
        {
            p.m_is_flying = !p.m_is_flying;

            if (p.m_is_flying)
            {
                p.vel.vy = 0.0f;
            }
        });

    m_action_binding[PlayerAction::BUILD_BLOCK].action = derived_action<player>(
        [this](const player& p, float dt)
        {
            if (p.m_world)
            {
                on_right_click();
            }
        });

    m_action_binding[PlayerAction::DESTROY_BLOCK].action = derived_action<player>(
        [this](const player& p, float dt)
        {
            if (p.m_world)
            {
                on_left_click();
            }
        });

    m_action_binding[PlayerAction::PLACE_LIGHT].action = derived_action<player>(
        [this](const player& p, float dt)
        {
            if (p.m_world)
            {
                on_light();
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
        hy > 0 && hy < 256 && item::is_destructable(hw))
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
        m_world->record_block(hx, hy, hz, 0);
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
        if (!m_world->player_intersects_block(2, s->x, s->y, s->z, hx, hy, hz))
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
        m_world->set_sign(hx, hy, hz, face, "words");
    }
}

float player::lerp(float a, float b, float t) noexcept
{
    return a + t * (b - a);
}
