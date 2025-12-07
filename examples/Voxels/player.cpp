#include "player.h"

#include <ranges>

#include "command_queue.h"
#include "entity.h"
#include "matrix.h"

player::player() : m_is_active{true}
{
    m_key_binding[SDL_SCANCODE_LEFT] = PlayerAction::MOVE_LEFT;
    m_key_binding[SDL_SCANCODE_RIGHT] = PlayerAction::MOVE_RIGHT;
    m_key_binding[SDL_SCANCODE_SPACE] = PlayerAction::JUMP;

    initialize_actions();

    for (auto& [action, category] : m_action_binding | std::views::values)
    {
        category = Entity::PLAYER;
    }
}

void player::handle_event(SDL_Event &event, command_queue &commands) noexcept
{
    static float dy = 0;
    state* s = &this->s1;
    int sz = 0;
    int sx = 0;
    float dir_mv = 0.025f;
    int sc = -1;

    if (event.type == SDL_EVENT_QUIT)
    {
        m_is_active = false;
    }
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        auto found = m_key_binding.find(event.key.scancode);

        if (found != m_key_binding.cend() && !is_realtime_action(found->second))
        {
            if (found->second == PlayerAction::JUMP)
            {
                SDL_Log("Player JUMP action (ignored - not on ground)");
                return; // do not jump if not on ground
            }
            SDL_Log("Player action queued: %d", static_cast<int>(found->second));
            commands.push(m_action_binding[found->second]);
        }
    }
    if (event.type == SDL_SCANCODE_RETURN)
    {
    }
    if (event.type == SDL_EVENT_MOUSE_MOTION)
    {
        constexpr auto WINDOW_W = 1020;
        constexpr auto WINDOW_H = 720;

        constexpr bool INVERT_MOUSE = false;
        constexpr float mouse_mv = 0.0025f;
        // Adjust mouse motion based on relative center of voxel_scene_size
        float adjusted_xrel = event.motion.xrel - static_cast<float>(WINDOW_W) / 2.f;
        float adjusted_yrel = event.motion.yrel - static_cast<float>(WINDOW_H) / 2.f;

        float old_rx = s->rx;
        float old_ry = s->ry;

        s->rx += event.motion.xrel * mouse_mv;
        if (INVERT_MOUSE) {
            s->ry += event.motion.yrel * mouse_mv;
        } else {
            s->ry -= event.motion.yrel * mouse_mv;
        }
        if (s->rx < 0) {
            s->rx += RADIANS(360);
        }
        if (s->rx >= RADIANS(360)) {
            s->rx -= RADIANS(360);
        }
        s->ry = SDL_max(s->ry, -RADIANS(90));
        s->ry = SDL_min(s->ry, RADIANS(90));

        // Debug: Log camera rotation changes periodically
        static int mouse_event_count = 0;
        if (mouse_event_count++ % 100 == 0) {
            SDL_Log("Camera rotation: rx=%.3f (delta=%.3f), ry=%.3f (delta=%.3f)",
                    s->rx, s->rx - old_rx, s->ry, s->ry - old_ry);
        }
    }
}
void player::handle_realtime_input(command_queue &commands)
{
    static int realtime_frame_count = 0;
    bool any_action = false;

    for (auto &pair : m_key_binding)
    {
        if (is_realtime_action(pair.second))
        {
            int numKeys = 0;
            const auto *keyState = SDL_GetKeyboardState(&numKeys);

            if (keyState && pair.first < static_cast<std::uint32_t>(numKeys) && keyState[pair.first])
            {
                commands.push(m_action_binding[pair.second]);
                any_action = true;

                // Debug: Log realtime actions periodically
                if (realtime_frame_count % 60 == 0) {
                    SDL_Log("Realtime action: %d (key=%d)", static_cast<int>(pair.second), pair.first);
                }
            }
        }
    }

    if (any_action) {
        realtime_frame_count++;
    }
}

void player::assign_key(PlayerAction action, std::uint32_t key)
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
void player::set_active(bool active) noexcept
{
    m_is_active = active;
}

std::uint32_t player::get_buffer() const noexcept
{
    return this->m_buffer;
}
void player::set_buffer(std::uint32_t value) noexcept
{
    this->m_buffer = value;
}

void player::initialize_actions()
{
    static constexpr auto playerSpeed = 200.f;
    static constexpr auto jumpForce = -500.f;

    // Note: derived_action is a member function of craft_impl,
    // so we'll use a simple lambda instead
    m_action_binding[PlayerAction::MOVE_LEFT].action = [](scene_node &node, float dt)
    {
        // Do something for move left action
    };

    // on create block

    // on destroy block

    // on copy block
}

bool player::is_realtime_action(PlayerAction action) noexcept
{
    switch (action)
    {
    case PlayerAction::MOVE_LEFT:
    case PlayerAction::MOVE_RIGHT:
        return true;
    default:
        return false;
    }
}
