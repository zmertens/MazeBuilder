#include "player.h"

#include <ranges>

#include "command_queue.h"
#include "entity.h"
#include "matrix.h"
#include "world.h"

#include <SDL3/SDL.h>

player::player()
    : scene_node{}
    , m_is_active{true}
    , m_on_ground{false}
, m_is_flying{false}
    , m_buffer{}
    , m_world{nullptr}
{
    set_category(Entity::PLAYER);

    // Movement key bindings
    m_key_binding[SDL_SCANCODE_A] = PlayerAction::MOVE_LEFT;
    m_key_binding[SDL_SCANCODE_D] = PlayerAction::MOVE_RIGHT;
    m_key_binding[SDL_SCANCODE_W] = PlayerAction::MOVE_FORWARD;
    m_key_binding[SDL_SCANCODE_S] = PlayerAction::MOVE_BACKWARD;
    m_key_binding[SDL_SCANCODE_SPACE] = PlayerAction::JUMP;  // Also used for UP in flying mode
    m_key_binding[SDL_SCANCODE_LSHIFT] = PlayerAction::MOVE_DOWN;  // Down in flying mode
    m_key_binding[SDL_SCANCODE_TAB] = PlayerAction::FLY;

    initialize_actions();

    for (auto& [_, category] : m_action_binding | std::views::values)
    {
        category = Entity::PLAYER;
    }
}

void player::handle_event(const SDL_Event &event, command_queue &commands) noexcept
{
    state* s = &this->s1;

    if (event.type == SDL_EVENT_QUIT)
    {
        m_is_active = false;
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
    if (event.type == SDL_EVENT_MOUSE_MOTION)
    {
        constexpr float mouse_sensitivity = 0.0025f;

        s->rx += event.motion.xrel * mouse_sensitivity;
        static constexpr auto INVERT_MOUSE = false;
        if (INVERT_MOUSE) {
            s->ry += event.motion.yrel * mouse_sensitivity;
        }
        s->ry -= event.motion.yrel * mouse_sensitivity;

        // Keep rotation within bounds
        if (s->rx < 0) {
            s->rx += RADIANS(360);
        }
        if (s->rx >= RADIANS(360)) {
            s->rx -= RADIANS(360);
        }
        s->ry = SDL_max(s->ry, -RADIANS(90));
        s->ry = SDL_min(s->ry, RADIANS(90));
    }
}
void player::handle_realtime_input(command_queue &commands)
{
    static int frame_counter = 0;
    bool any_key_pressed = false;

    for (auto & [fst, snd] : m_key_binding)
    {
        // Regular realtime actions OR JUMP when flying
        if (is_realtime_action(snd) || (snd == PlayerAction::JUMP && m_is_flying))
        {
            int numKeys = 0;

            if (const auto *keyState = SDL_GetKeyboardState(&numKeys);
                keyState && fst < static_cast<std::uint32_t>(numKeys) && keyState[fst])
            {
                commands.push(m_action_binding[snd]);
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
            it = m_key_binding.erase(it);
        else
            ++it;
    }

    // Insert new binding
    m_key_binding[key] = action;
}

[[nodiscard]] std::uint32_t player::get_assigned_key(const PlayerAction action) const
{
    for (const auto & [f, s] : m_key_binding)
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

std::uint32_t player::get_buffer() const noexcept
{
    return this->m_buffer;
}

void player::set_buffer(const std::uint32_t value) noexcept
{
    this->m_buffer = value;
}

void player::set_world(world* w) noexcept
{
    m_world = w;
}

bool player::is_on_ground() const noexcept
{
    return m_on_ground;
}

void player::initialize_actions()
{
    m_action_binding[PlayerAction::MOVE_BACKWARD].action = derived_action<player>(
        [](player& p, float dt)
        {
            if (!p.m_world) {
                SDL_Log("MOVE_FORWARD: No world reference!");
                return;
            }

            constexpr float moveSpeed = 0.1f;
            float old_x = p.s1.x;
            float old_z = p.s1.z;
            float dx = -SDL_sinf(p.s1.rx) * moveSpeed;
            float dz = SDL_cosf(p.s1.rx) * moveSpeed;

            p.s1.x += dx;
            p.s1.z += dz;

            static int move_count = 0;
            if (move_count++ % 60 == 0) {
                SDL_Log("MOVE_FORWARD executed: (%.2f, %.2f) -> (%.2f, %.2f)", old_x, old_z, p.s1.x, p.s1.z);
            }
        });

    m_action_binding[PlayerAction::MOVE_FORWARD].action = derived_action<player>(
        [](player& p, float dt)
        {
            if (!p.m_world) return;

            constexpr float moveSpeed = 0.1f;
            const float dx = SDL_sinf(p.s1.rx) * moveSpeed;
            const float dz = -SDL_cosf(p.s1.rx) * moveSpeed;

            p.s1.x += dx;
            p.s1.z += dz;
        });

    m_action_binding[PlayerAction::MOVE_LEFT].action = derived_action<player>(
        [](player& p, float dt)
        {
            if (!p.m_world) return;

            constexpr float moveSpeed = 0.1f;
            const float dx = -SDL_cosf(p.s1.rx) * moveSpeed;
            const float dz = -SDL_sinf(p.s1.rx) * moveSpeed;

            p.s1.x += dx;
            p.s1.z += dz;
        });

    m_action_binding[PlayerAction::MOVE_RIGHT].action = derived_action<player>(
        [](player& p, float dt)
        {
            if (!p.m_world) return;

            constexpr float moveSpeed = 0.1f;
            float dx = SDL_cosf(p.s1.rx) * moveSpeed;
            float dz = SDL_sinf(p.s1.rx) * moveSpeed;

            p.s1.x += dx;
            p.s1.z += dz;
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

    // MOVE_DOWN action for flying mode (Left Shift)
    m_action_binding[PlayerAction::MOVE_DOWN].action = derived_action<player>(
        [](player& p, float dt)
        {
            if (p.m_is_flying)
            {
                // In flying mode, move down
                constexpr float flySpeed = 0.15f;
                p.vel.vy = -flySpeed;
            }
        });

    m_action_binding[PlayerAction::FLY].action = derived_action<player>(
    [](player& p, float dt)
    {
        p.m_is_flying = !p.m_is_flying;

        if (p.m_is_flying)
        {
            // When entering flying mode, zero out vertical velocity
            p.vel.vy = 0.0f;
            SDL_Log("Flying mode: ENABLED");
        }
        else
        {
            SDL_Log("Flying mode: DISABLED");
        }
    });

    m_action_binding[PlayerAction::BUILD_BLOCK].action = derived_action<player>(
        [](player& p, float dt)
        {
            if (p.m_world)
            {
                p.m_world->on_right_click();
            }
        });

    m_action_binding[PlayerAction::DESTROY_BLOCK].action = derived_action<player>(
        [](player& p, float dt)
        {
            if (p.m_world)
            {
                p.m_world->on_left_click();
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
    case PlayerAction::MOVE_DOWN:  // Hold Shift to descend in flying mode
        return true;
    default:
        return false;
    }
}
