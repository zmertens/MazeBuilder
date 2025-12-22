#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <string>

#include <MazeBuilder/configurator.h>

#include "command.h"

class texture;

enum class PlayerAction
{
    MOVE_AUTO,
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
    COPY_BLOCK,
    DESTROY_BLOCK,
    PLACE_LIGHT,
    PLACE_MAZE,
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

class player : public scene_node
{
public:
    struct position
    {
        float x, y, z, rx, ry, t;
    } m_pos{};

    struct velocity
    {
        float vx, vy, vz;
    } m_vel{};

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
        std::uint64_t start_time{ };
        std::uint64_t start_ticks{ };
        mazes::configurator maze{ };
        std::string tag;
        bool preview_enabled{ true };
        bool download_ready{ false };
    } m_configs{};

    explicit player();

    ~player() override = default;

    player(const player&) = delete;
    player& operator=(const player&) = delete;

    player(player&&) noexcept = delete;
    player& operator=(player&&) = delete;

    void handle_event(const SDL_Event& event, command_queue& commands) noexcept;

    void update(float delta_time, mazes::randomizer& rng) noexcept;

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

    bool generate_maze_texture(mazes::grid_interface* g, mazes::randomizer& rng) noexcept;

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
    bool m_is_on_auto_run;

    std::string m_name;
    std::uint32_t m_buffer;

    std::int32_t m_item_index;

    world* m_world;

    std::function<std::unique_ptr<mazes::grid_interface>(const mazes::configurator&)> m_maze_task;

    std::unique_ptr<mazes::grid_factory> m_grid_factory;

    std::uint64_t m_last_maze_generation_time{ 0 };
    static constexpr std::uint64_t MAZE_GENERATION_COOLDOWN_MS{ 10000 };

    // Store maze data for artifacts generation
    mutable std::unique_ptr<mazes::grid_interface> m_last_maze_for_artifacts;
    mutable std::mutex m_maze_artifacts_mutex;
};

#endif // PLAYER_H
