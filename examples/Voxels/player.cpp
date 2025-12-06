#include "player.h"

#include <ranges>

#include "command_queue.h"
#include "entity.h"
// #include "scene_node.h"

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
                return; // do not jump if not on ground
            }
            commands.push(m_action_binding[found->second]);
        }
    }
    if (event.type == SDL_SCANCODE_RETURN)
    {
    }
}
void player::handle_realtime_input(command_queue &commands)
{
    for (auto &pair : m_key_binding)
    {
        if (is_realtime_action(pair.second))
        {
            int numKeys = 0;
            const auto *keyState = SDL_GetKeyboardState(&numKeys);

            if (keyState && pair.first < static_cast<std::uint32_t>(numKeys) && keyState[pair.first])
            {
                commands.push(m_action_binding[pair.second]);
            }
        }
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

[[nodiscard]] std::uint32_t player::get_assigned_key(PlayerAction action) const
{
    for (auto &pair : m_key_binding)
    {
        if (pair.second == action)
            return pair.first;
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
