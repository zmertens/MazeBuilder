#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <map>
#include <string>

#include <SDL3/SDL.h>

#include "command.h"

enum class PlayerAction
{
    MOVE_LEFT,
    MOVE_RIGHT,
    JUMP,
    COUNT
};

class command_queue;
class scene_node;

class player
{
public:
    struct state
    {
        float x, y, z, rx, ry, t;
    } s1, s2, s3;

    explicit player();

    ~player() = default;

    player(const player&) = delete;
    player& operator=(const player&) = delete;

    player(player&&) = default;
    player& operator=(player&&) = default;

    void handle_event(SDL_Event &event, command_queue &commands) noexcept;

    void handle_realtime_input(command_queue &commands);

    void assign_key(PlayerAction action, std::uint32_t key);

    [[nodiscard]] std::uint32_t get_assigned_key(PlayerAction action) const;

    bool is_active() const noexcept;

    void set_active(bool active) noexcept;

private:
    void initialize_actions();

    static bool is_realtime_action(PlayerAction action) noexcept;

    std::map<std::uint32_t, PlayerAction> m_key_binding;

    std::map<PlayerAction, command> m_action_binding;

    bool m_is_active;

    std::string m_name;
    std::uint32_t m_buffer;
};

#endif // PLAYER_H
