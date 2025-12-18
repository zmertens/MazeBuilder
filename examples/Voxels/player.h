#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <functional>
#include <future>
#include <map>
#include <string>

#include <MazeBuilder/algo_interface.h>
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
    BUILD_MAZE,
    PREVIEW_MAZE,
    DONE,
    COUNT
};

class command_queue;
union SDL_Event;
class world;

namespace mazes
{
    class grid_interface;
    class grid_factory;
    class randomizer;
}

class player : public scene_node, mazes::algo_interface
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
        bool show_download_button{ false };
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
        std::uint32_t maze_texture_id{ 0 };
        int maze_texture_width{ 0 };
        int maze_texture_height{ 0 };
        bool maze_ready{ false };
        bool preview_enabled{ true };
        bool download_ready{ false };
    } m_configs{};

    struct projected_plane
    {
        bool visible{ false };
        std::uint32_t texture_id{ 0 };
        int target_x{ 0 };
        int target_y{ 0 };
        int target_z{ 0 };
        int target_face{ 0 };
        bool has_valid_target{ false };
    } m_projected_plane{};

    explicit player();

    ~player() override = default;

    player(const player&) = delete;
    player& operator=(const player&) = delete;

    player(player&&) noexcept = default;
    player& operator=(player&&) = default;

    void handle_event(const SDL_Event& event, command_queue& commands) noexcept;

    void draw() const noexcept;

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

    [[nodiscard]] std::string get_name() const noexcept;
    void set_name(const std::string& name) noexcept;

    void set_world(world* w) noexcept;

    [[nodiscard]] std::string get_local_time() const noexcept;

    [[nodiscard]] const projected_plane& get_projected_plane() const noexcept;

    bool run(mazes::grid_interface* g, mazes::randomizer& rng) const noexcept override;

    bool generate_maze_texture(mazes::randomizer& rng) noexcept;

    std::string artifacts() const noexcept;

    [[nodiscard]] bool is_maze_generation_ready() const noexcept;
    [[nodiscard]] std::uint64_t get_maze_cooldown_remaining_ms() const noexcept;
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

    std::function<std::unique_ptr<mazes::grid_interface>(const mazes::configurator&)> m_maze_task;
    mutable std::future<std::unique_ptr<mazes::grid_interface>> m_maze_future;

    std::unique_ptr<mazes::grid_factory> m_grid_factory;

    // Cooldown management (timestamp-based, non-blocking)
    std::uint64_t m_last_maze_generation_time{ 0 };
    static constexpr std::uint64_t MAZE_GENERATION_COOLDOWN_MS{ 10000 }; // 10 seconds
};

#endif // PLAYER_H
