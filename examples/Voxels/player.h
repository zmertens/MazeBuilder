#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <map>
#include <string>

#include <MazeBuilder/configurator.h>

#include "command.h"

enum class PlayerAction
{
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_UP,
    MOVE_DOWN,
    JUMP,
    FLY,
    TAG_SIGN,
    BUILD_BLOCK,
    DESTROY_BLOCK,
    PLACE_LIGHT,
    DONE,
    COUNT
};

class command_queue;
union SDL_Event;
class world;

class player : public scene_node
{
public:
    struct position
    {
        float x, y, z, rx, ry, t;
    } pos{};

    struct velocity
    {
        float vx, vy, vz;
    } vel{};

    struct configs
    {
        bool fullscreen{ false };
        bool invert_mouse{ false };
        bool show_stats_window{ true };
        bool use_bloom_effect{ false };
        bool vsync{ true };
        float exposure_range{ 0.5f };
        float fov{ };
        int day_length{ };
        int ortho{ 0 };
        int start_time{ };
        int start_ticks{ };
        mazes::configurator maze{ };
        std::string tag;
    } m_configs{};

    explicit player();

    ~player() override = default;

    player(const player&) = delete;
    player& operator=(const player&) = delete;

    player(player&&) noexcept = default;
    player& operator=(player&&) = default;

    void handle_event(const SDL_Event& event, command_queue& commands) noexcept;

    void handle_realtime_input(command_queue& commands);

    void assign_key(PlayerAction action, std::uint32_t key);

    [[nodiscard]] std::uint32_t get_assigned_key(PlayerAction action) const;

    [[nodiscard]] bool is_active() const noexcept;
    void set_active(bool active) noexcept;

    [[nodiscard]] bool is_flying() const noexcept;
    void set_flying(bool flying) noexcept;

    [[nodiscard]] bool is_on_ground() const noexcept;
    void set_on_ground(bool grounded) noexcept;

    [[nodiscard]] std::uint32_t get_buffer() const noexcept;
    void set_buffer(std::uint32_t value) noexcept;

    [[nodiscard]] std::int32_t get_item() const noexcept;
    void set_item(std::int32_t value) noexcept;

    void set_world(world* w) noexcept;

    std::string get_local_time() const noexcept;

private:
    void initialize_actions();
    static bool is_realtime_action(PlayerAction action) noexcept;

    void on_light() const noexcept;
    void on_left_click() const noexcept;
    void on_right_click() const noexcept;
    void on_middle_click() noexcept;
    void on_tag_sign() const noexcept;

    static float lerp(float a, float b, float t) noexcept;

    std::map<std::uint32_t, PlayerAction> m_key_binding;

    std::map<PlayerAction, command> m_action_binding;

    bool m_is_active;
    bool m_on_ground;
    bool m_is_flying;
    bool m_is_ctrl_held;

    std::string m_name;
    std::uint32_t m_buffer;

    std::int32_t m_item_index;

    world* m_world;
};

#endif // PLAYER_H
