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

// Base class for all game objects that can interact in the world
class scene_node
{
public:
    virtual ~scene_node() = default;

    // Chunk-specific data
    voxels_map map;
    voxels_map lights;
    sign_list signs;
    int p;
    int q;
    int faces;
    int sign_faces;
    int dirty;
    int miny;
    int maxy;
    std::uint32_t buffer;
    std::uint32_t sign_buffer;

    scene_node *parent;
    std::vector<scene_node *> children;

    // Spatial bounds (for quadtree nodes)
    int bounds_min_x;
    int bounds_min_z;
    int bounds_max_x;
    int bounds_max_z;

    [[nodiscard]] Entity get_category() const noexcept
    {
        return m_category;
    }

    void set_category(const Entity category) noexcept
    {
        m_category = category;
    }

    // Check if this node or its children intersect with a 2D bounds (for frustum culling)
    [[nodiscard]] bool intersects_bounds(const int min_x, const int min_z,
                                         const int max_x, const int max_z) const noexcept
    {
        return !(bounds_max_x < min_x || bounds_min_x > max_x ||
                 bounds_max_z < min_z || bounds_min_z > max_z);
    }

private:
    Entity m_category;
};

namespace mazes
{
    class randomizer;
}

struct command
{
    std::function<void(scene_node &, float, mazes::randomizer &rng)> action;
    Entity category;
};

using command_queue = std::queue<command>;

template <typename GameObject, typename Function>
std::function<void(scene_node &, float, mazes::randomizer &)> derived_action(Function fn)
{
    return [=](scene_node &node, float dt, mazes::randomizer &rng)
    {
        // Ensure that the cast is safe - check if scene_node is base of GameObject
        if constexpr (std::is_base_of_v<scene_node, GameObject>)
        {
            fn(static_cast<GameObject &>(node), dt, std::ref(rng));
        }
    };
}

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
class world;

namespace mazes
{
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

    enum class OrthoViewMode : int
    {
        PERSPECTIVE = 0,
        ISOMETRIC = 1,
        FIXED_INT_FOR_ISO_VIEW = 32
    };

    enum class CADTool : int
    {
        NONE = 0,
        MEASURE_DISTANCE = 1,
        MEASURE_AREA = 2,
        SELECT_BOX = 3
    };

    struct measurement_point
    {
        int x{0}, y{0}, z{0};
        bool valid{false};
    };

    struct configs
    {
        bool fullscreen{false};
        bool invert_mouse{false};
        bool show_stats_window{false};
        bool use_bloom_effect{true};
        bool vsync{true};
        float exposure_range{0.5f};
        float fov{};
        int day_length{};
        int ortho{1};
        std::uint64_t start_time{};
        std::uint64_t start_ticks{};
        mazes::configurator maze{};
        std::string tag;
        bool preview_enabled{true};
        bool artifacts_ready{false};
        float gui_font_scale{1.0f};

        // CAD Features (Tier 1)
        bool show_hover_info{true};
        bool show_grid{false};
        bool enable_grid_snap{false};
        bool show_maze_preview_ghost{true};
        OrthoViewMode ortho_view_mode{OrthoViewMode::ISOMETRIC};
        int grid_spacing{1};
        float grid_opacity{0.3f};
        CADTool active_cad_tool{CADTool::NONE};
    } m_configs{};

    // CAD Tool State
    measurement_point m_measure_point1{};
    measurement_point m_measure_point2{};

    explicit player();

    ~player() override = default;

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
    [[nodiscard]] static const char *get_block_name(int block_type) noexcept;
    [[nodiscard]] static const char *get_face_name(int face) noexcept;
    void cycle_ortho_view() noexcept;
    void toggle_grid() noexcept;
    void toggle_hover_display() noexcept;
    void activate_measurement_tool() noexcept;
    void record_measurement_point(int x, int y, int z) noexcept;
    void clear_measurement() noexcept;

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

    std::unordered_map<std::uint32_t, PlayerAction> m_key_binding;

    std::unordered_map<PlayerAction, command> m_action_binding;

    bool m_is_active;
    bool m_on_ground;
    bool m_is_flying;
    bool m_is_on_auto_run;

    std::string m_name;
    std::uint32_t m_buffer;

    std::int32_t m_item_index;

    world *m_world;

    std::function<std::optional<maze_preview_frame>(const mazes::configurator &)> m_maze_task;
    std::future<std::optional<maze_preview_frame>> m_preview_future;
#if defined(__EMSCRIPTEN__)
    std::optional<maze_preview_frame> m_pending_preview;
#endif

    std::uint64_t m_last_preview_generation_time{0};
    std::uint64_t m_last_preview_request_time{0};
    bool m_auto_preview_pending{true};

    // Async artifact export
    std::future<std::string> m_artifact_export_future;
    std::string m_cached_artifact_result;
};

#endif // PLAYER_H
