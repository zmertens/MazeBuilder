#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <future>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <unordered_map>
#include <string>
#include <type_traits>
#include <vector>

#include <MazeBuilder/configurator.h>

#include "db.h"
#include "geometries.h"
#include "item.h"
#include "voxels_map.h"

class texture;

enum class Entity : unsigned int
{
    NONE = 0,
    SCENE = 1 << 0,
    PLAYER = 1 << 1,
    ENEMY = 1 << 2,
    PROJECTILE = 1 << 3,
    PICKUP = 1 << 4,
    CHUNK = 1 << 5,
    SPATIAL = 1 << 6,
    ALL = 1 << 7
};

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
    CHANGE_PERSPECTIVE,
    ZOOM_IN_ISO_VIEW,
    ZOOM_OUT_ISO_VIEW,
    DONE,
    COUNT
};

union SDL_Event;

class scene_node;
class world;

namespace mazes
{
    class randomizer;
}

class player;

struct command
{
    std::function<void(player &, float, mazes::randomizer &rng)> action;
    Entity category;
};

using command_queue = std::queue<command>;

template <typename GameObject, typename Function>
std::function<void(player &, float, mazes::randomizer &)> derived_action(Function fn)
{
    return [=](player &p, float dt, mazes::randomizer &rng)
    {
        // Ensure that the cast is safe - check if player is base of GameObject
        if constexpr (std::is_base_of_v<player, GameObject>)
        {
            fn(static_cast<GameObject &>(p), dt, std::ref(rng));
        }
    };
}

class player
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

    enum class PlayerViewMode : int
    {
        PERSPECTIVE = 0,
        ISOMETRIC = 1,
        COUNT = 2,
        FIXED_INT_FOR_ORTHO_SCALING = 32
    };

    enum class CADTool : int
    {
        NONE = 0,
        SELECT_BOX = 2
    };

    struct configs
    {
        configs &artifacts_ready(bool ready)
        {
            opt_artifacts_ready = ready;
            return *this;
        }

        configs &enable_grid_snap(bool enabled)
        {
            opt_enable_grid_snap = enabled;
            return *this;
        }

        configs &fullscreen(bool enabled)
        {
            opt_fullscreen = enabled;
            return *this;
        }

        configs &invert_mouse(bool enabled)
        {
            opt_invert_mouse = enabled;
            return *this;
        }

        configs &show_grid_overlay(bool enabled)
        {
            opt_show_grid_overlay = enabled;
            return *this;
        }

        configs &show_heightmap(bool enabled)
        {
            opt_show_heightmap = enabled;
            return *this;
        }

        configs &show_crosshair_details(bool enabled)
        {
            opt_show_crosshair_details = enabled;
            return *this;
        }

        configs &show_maze_preview_2d_enabled(bool enabled)
        {
            opt_show_maze_preview_2d_enabled = enabled;
            return *this;
        }

        configs &show_maze_preview_ghost(bool enabled)
        {
            opt_show_maze_preview_ghost = enabled;
            return *this;
        }

        configs &show_stats_window(bool enabled)
        {
            opt_show_stats_window = enabled;
            return *this;
        }

        configs &use_bloom_effect(bool enabled)
        {
            opt_use_bloom_effect = enabled;
            return *this;
        }

        configs &vsync(bool enabled)
        {
            opt_vsync = enabled;
            return *this;
        }

        configs &active_cad_tool(CADTool tool)
        {
            opt_cad_tool = tool;
            return *this;
        }

        configs &exposure_range(float exposure)
        {
            opt_exposure_range = exposure;
            return *this;
        }

        configs &fov(float value)
        {
            opt_fov = value;
            return *this;
        }

        configs &gui_font_scale(float scale)
        {
            opt_gui_font_scale = scale;
            return *this;
        }

        configs &day_length(int length)
        {
            opt_day_length = length;
            return *this;
        }

        configs &grid_spacing(int spacing)
        {
            opt_grid_spacing = spacing;
            return *this;
        }

        configs &grid_opacity(float opacity)
        {
            opt_grid_opacity = opacity;
            return *this;
        }

        configs &ortho_scaling(int scaling)
        {
            opt_ortho_scaling = static_cast<PlayerViewMode>(scaling);
            return *this;
        }

        configs &maze(const mazes::configurator &maze_config)
        {
            opt_maze = maze_config;
            return *this;
        }

        configs &player_view_mode(PlayerViewMode view_mode)
        {
            opt_player_view_mode = view_mode;
            return *this;
        }

        configs &tag(const std::string &value)
        {
            opt_tag = value;
            return *this;
        }

        configs &start_time(std::uint64_t time)
        {
            opt_start_time = time;
            return *this;
        }

        configs &start_ticks(std::uint64_t ticks)
        {
            opt_start_ticks = ticks;
            return *this;
        }

        bool artifacts_ready() const noexcept
        {
            return opt_artifacts_ready.value_or(false);
        }

        bool enable_grid_snap() const noexcept
        {
            return opt_enable_grid_snap.value_or(false);
        }

        bool fullscreen() const noexcept
        {
            return opt_fullscreen.value_or(false);
        }

        bool invert_mouse() const noexcept
        {
            return opt_invert_mouse.value_or(false);
        }

        bool show_grid_overlay() const noexcept
        {
            return opt_show_grid_overlay.value_or(false);
        }

        bool show_heightmap() const noexcept
        {
            return opt_show_heightmap.value_or(false);
        }

        bool show_crosshair_details() const noexcept
        {
            return opt_show_crosshair_details.value_or(true);
        }

        bool show_maze_preview_2d_enabled() const noexcept
        {
            return opt_show_maze_preview_2d_enabled.value_or(true);
        }

        bool show_maze_preview_ghost() const noexcept
        {
            return opt_show_maze_preview_ghost.value_or(true);
        }

        bool show_stats_window() const noexcept
        {
            return opt_show_stats_window.value_or(true);
        }

        bool use_bloom_effect() const noexcept
        {
            return opt_use_bloom_effect.value_or(true);
        }

        bool vsync() const noexcept
        {
            return opt_vsync.value_or(true);
        }

        CADTool active_cad_tool() const noexcept
        {
            return opt_cad_tool.value_or(CADTool::NONE);
        }

        float exposure_range() const noexcept
        {
            return opt_exposure_range.value_or(0.45f);
        }

        float fov() const noexcept
        {
            return opt_fov.value_or(60.0f);
        }

        float grid_opacity() const noexcept
        {
            return opt_grid_opacity.value_or(1.0f);
        }

        float gui_font_scale() const noexcept
        {
            return opt_gui_font_scale.value_or(1.0f);
        }

        int day_length() const noexcept
        {
            return opt_day_length.value_or(600);
        }

        int grid_spacing() const noexcept
        {
            return opt_grid_spacing.value_or(8);
        }

        int ortho_scaling() const noexcept
        {
            return static_cast<int>(opt_ortho_scaling.value_or(PlayerViewMode::FIXED_INT_FOR_ORTHO_SCALING));
        }

        mazes::configurator maze() const noexcept
        {
            return opt_maze.value_or(mazes::configurator{});
        }

        PlayerViewMode player_view_mode() const noexcept
        {
            return opt_player_view_mode.value_or(PlayerViewMode::ISOMETRIC);
        }

        std::string tag() const noexcept
        {
            return opt_tag.value_or("Put maze here");
        }

        std::uint64_t start_time() const noexcept
        {
            return opt_start_time.value_or(0);
        }

        std::uint64_t start_ticks() const noexcept
        {
            return opt_start_ticks.value_or(0);
        }

private:
        std::optional<bool> opt_artifacts_ready{false};
        std::optional<bool> opt_enable_grid_snap{false};
        std::optional<bool> opt_fullscreen{false};
        std::optional<bool> opt_show_crosshair_details{true};
        std::optional<bool> opt_invert_mouse{false};
        std::optional<bool> opt_show_grid_overlay{false};
        std::optional<bool> opt_show_heightmap{false};
        std::optional<bool> opt_show_maze_preview_2d_enabled{true};
        std::optional<bool> opt_show_maze_preview_ghost{true};
        std::optional<bool> opt_show_stats_window{true};
        std::optional<bool> opt_use_bloom_effect{true};
        std::optional<bool> opt_vsync{false};

        std::optional<CADTool> opt_cad_tool{CADTool::NONE};

        std::optional<float> opt_exposure_range{0.45f};
        std::optional<float> opt_fov{60.0f};
        std::optional<float> opt_grid_opacity{1.0f};
        std::optional<float> opt_gui_font_scale{1.0f};

        std::optional<int> opt_day_length{600};
        std::optional<int> opt_grid_spacing{8};

        std::optional<mazes::configurator> opt_maze{};

        std::optional<PlayerViewMode> opt_ortho_scaling{PlayerViewMode::FIXED_INT_FOR_ORTHO_SCALING};
        std::optional<PlayerViewMode> opt_player_view_mode{PlayerViewMode::ISOMETRIC};

        std::optional<std::string> opt_tag{"Put maze here"};

        std::optional<std::uint64_t> opt_start_time{0};
        std::optional<std::uint64_t> opt_start_ticks{0};
    } _configs{};

    explicit player();

    ~player() = default;

    player(const player &) = delete;
    player &operator=(const player &) = delete;

    player(player &&) noexcept = delete;
    player &operator=(player &&) = delete;

    void handle_event(const SDL_Event &event, command_queue &commands) noexcept;

    void update(float delta_time, mazes::randomizer &rng) noexcept;

    void draw() const noexcept;

    void handle_realtime_input(command_queue &commands);

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
    void set_name(const std::string &name) noexcept;

    void set_world(world *w) noexcept;

    [[nodiscard]] std::string get_local_time() const noexcept;

    [[nodiscard]] std::string artifacts() const noexcept;

    [[nodiscard]] bool is_download_ready() const noexcept;

    // Async artifact export
    void start_async_artifact_export() noexcept;
    [[nodiscard]] bool is_artifact_export_ready() const noexcept;
    [[nodiscard]] std::string get_artifact_export_result() noexcept;

    // CAD Helper Functions
    [[nodiscard]] static const char *get_block_name(item::BlockType block_type) noexcept;
    [[nodiscard]] static const char *get_face_name(int face) noexcept;
    void cycle_ortho_view() noexcept;
    void toggle_grid_overlay() noexcept;
    void toggle_crosshair_details() noexcept;
    void activate_measurement_tool() noexcept;
    void record_measurement_point(int x, int y, int z) noexcept;
    void clear_measurement() noexcept;

    void set_font_index(std::int32_t index) noexcept;
    [[nodiscard]] std::int32_t get_font_index() const noexcept;

private:
    void initialize_actions();
    static bool is_realtime_action(PlayerAction action) noexcept;
    bool preview_generation_in_progress() const noexcept;
    bool request_preview_generation() noexcept;
    void process_preview_generation() noexcept;

    void on_light() const noexcept;
    void on_left_click() const noexcept;
    void on_right_click() const noexcept;
    void on_middle_click() noexcept;
    void on_tag_sign() const noexcept;

    static float lerp(float a, float b, float t) noexcept;

    std::unordered_map<std::uint32_t, PlayerAction> keyboard_bindings;

    std::unordered_map<PlayerAction, command> player_commands;

    bool _is_active;
    bool _is_on_ground;
    bool _is_flying;
    bool _is_auto_running;

    std::string my_name;
    std::uint32_t player_buffer;

    std::int32_t current_item_index;
    std::int32_t selected_font_index;

    world *current_voxel_world;

    std::function<std::optional<maze_preview_frame>(const mazes::configurator &)> generate_maze_task;
    std::future<std::optional<maze_preview_frame>> preview_maze_2d_fut;
#if defined(__EMSCRIPTEN__)
    std::optional<maze_preview_frame> pending_maze_preview_2d;
#endif

    std::uint64_t last_preview_generation_time{0};
    std::uint64_t last_preview_request_time{0};
    bool auto_preview_pending{true};

    // Async artifact export
    std::future<std::string> artifact_export_fut;
    std::string artifact_cache_results;
};

#endif // PLAYER_H
