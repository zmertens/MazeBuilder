#ifndef WORLD_H
#define WORLD_H

#include <array>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "command_queue.h"

#include "map.h"
#include "resource_identifiers.h"
#include "sdl_gl_helper.h"
#include "sign.h"

enum class WorkerState : int
{
    IDLE = 0,
    BUSY = 1,
    DONE = 2
};

union SDL_Event;

std::string gl_error_checker(const char* file, int line) noexcept;

#define CHECK_GL_ERR() gl_error_checker(__FILE__, __LINE__)

using world_func = std::function<void(int, int, int, int, Map*)>;

class attrib;
class command_queue;
class player;
class sdl_gl_helper;

namespace mazes {
    class randomizer;
}

class world final {
public:
    explicit world(SDL_Window* window,
        font_manager& fonts,
        player* p,
        shader_manager& shaders,
        texture_manager& textures,
        const sdl_gl_helper* sdl);

    ~world();

    void init() noexcept;

    void update(float dt, mazes::randomizer& rng) noexcept;

    void draw() const noexcept;

    command_queue& get_command_queue() noexcept;

    // Destroy the world
    void destroy_world();

    void handle_event(const SDL_Event& event) noexcept;

    static void create_world(int p, int q, world_func func, Map *m, int chunk_size) noexcept;

    // Block manipulation methods (public for player actions)
    void on_light() const noexcept;
    void on_left_click() noexcept;
    void on_right_click() noexcept;
    void on_middle_click() noexcept;

private:
    // Build the scene (initialize scene graph and layers)
    void build_scene();

    // Scene graph helper methods
    void attach_chunk_to_layer(scene_node* chunk, int layer_index) noexcept;
    void detach_chunk_from_layer(scene_node* chunk) noexcept;
    void traverse_chunks(const std::function<void(scene_node*)>& callback) const noexcept;
    void traverse_chunks_in_bounds(int min_p, int min_q, int max_p, int max_q,
                                     const std::function<void(scene_node*)>& callback) const noexcept;
    void insert_chunk_into_spatial_tree(scene_node* chunk) const noexcept;
    void remove_chunk_from_spatial_tree(scene_node* chunk) noexcept;

    struct worker_item {
        int p{};
        int q{};
        int load{};
        Map* block_maps[3][3]{};
        Map* light_maps[3][3]{};
        int miny{};
        int maxy{};
        int faces{};
        float* data{};
    };

    struct worker {
        int index;
        WorkerState state;
        std::thread thrd;
        std::mutex mtx;
        std::condition_variable cnd;
        worker_item item;
        bool should_stop;
    };

    typedef struct {
        int x;
        int y;
        int z;
        int w;
    } Block;

    struct model {
        int sign_radius;
        bool flying;
        int item_index;
        int scale;
        bool is_ortho;
        float fov;
        int day_length;
        int start_time;
        int start_ticks;
        Block block0;
        Block block1;
        Block copy0;
        Block copy1;
    };

    bool worker_run(worker* w) const noexcept;
    void init_worker_threads() noexcept;
    void cleanup_worker_threads() noexcept;

    [[nodiscard]] static int chunked(float x) noexcept;

    [[nodiscard]] double get_time() const noexcept;
    [[nodiscard]] float time_of_day() const noexcept;
    [[nodiscard]] float get_daylight() const noexcept;

    [[nodiscard]] std::uint32_t gen_crosshair_buffer() const noexcept;
    [[nodiscard]] std::uint32_t gen_wireframe_buffer(float x, float y, float z, float n) const noexcept;
    [[nodiscard]] std::uint32_t gen_cube_buffer(float x, float y, float z, float n, int w) const noexcept;
    [[nodiscard]] std::uint32_t gen_plant_buffer(float x, float y, float z, float n, int w) const noexcept;
    [[nodiscard]] std::uint32_t gen_player_buffer(float x, float y, float z, float rx, float ry) const noexcept;
    [[nodiscard]] std::uint32_t gen_text_buffer(float x, float y, float n, std::string_view text) const noexcept;

    [[nodiscard]] std::optional<scene_node*> find_chunk(int p, int q) const noexcept;
    static int chunk_distance(const scene_node* chunk, int p, int q) noexcept;
    int chunk_visible(float planes[6][4], int p, int q, int miny, int maxy) const noexcept;

    [[nodiscard]] int highest_block(float x, float z) const noexcept;
    static int _hit_test(const Map* map, float max_distance, int previous,
        float x, float y, float z, float vx, float vy, float vz, int* hx, int* hy, int* hz) noexcept;
    int hit_test(int previous, float x, float y, float z, float rx, float ry, int* bx, int* by, int* bz) const noexcept;
    int hit_test_face(player* _player, int* x, int* y, int* z, int* face) const noexcept;
    int collide(int height, float* x, float* y, float* z) const noexcept;
    [[nodiscard]] int player_intersects_block(int height, float x, float y, float z, int hx, int hy, int hz) const noexcept;

    int _gen_sign_buffer(float* data, float x, float y, float z, int face, std::string_view text) const noexcept;
    void gen_sign_buffer(scene_node* chunk) const noexcept;

    int has_lights(scene_node* chunk) const noexcept;

    void dirty_chunk(scene_node* chunk) const noexcept;
    // Process dirty chunks on worker threads
    void update_dirty_chunks_async() noexcept;

    static void occlusion(char neighbors[27], char lights[27], float shades[27], float ao[6][4], float light[6][4]) noexcept;
    static void light_fill(char* opaque, char* light, int x, int y, int z, int w, int force) noexcept;

    void compute_chunk(worker_item* item) const noexcept;

    void generate_chunk(scene_node* chunk, worker_item* item) const noexcept;
    void gen_chunk_buffer(scene_node* chunk) const noexcept;

    static void map_set_func(int x, int y, int z, int w, Map* m) noexcept;

    void load_chunk(worker_item* item) const noexcept;
    void init_chunk(scene_node* chunk, int p, int q) const noexcept;
    void create_chunk(scene_node* chunk, int p, int q) const noexcept;
    void delete_chunks() noexcept;
    void delete_all_chunks() noexcept;
    void force_chunks(player* player) noexcept;
    void check_workers() noexcept;
    void ensure_chunks_worker(player* _player, worker* w) noexcept;
    void ensure_chunks(player* _player) noexcept;

    void unset_sign(int x, int y, int z) const noexcept;
    void unset_sign_face(int x, int y, int z, int face) const noexcept;
    void _set_sign(int p, int q, int x, int y, int z, int face, std::string_view text, int dirty) const noexcept;
    void set_sign(int x, int y, int z, int face, std::string_view text) const noexcept;

    void toggle_light(int x, int y, int z) const noexcept;
    void set_light(int p, int q, int x, int y, int z, int w) const noexcept;

    void _set_block(int p, int q, int x, int y, int z, int w, int dirty) const noexcept;
    void set_block(int x, int y, int z, int w) const noexcept;
    void record_block(int x, int y, int z, int w) noexcept;
    int get_block(int x, int y, int z) noexcept;
    void builder_block(int x, int y, int z, int w) noexcept;

    int render_chunks(const sdl_gl_helper::attrib* attrib, player* _player, uint32_t texture) const noexcept;
    void render_signs(const sdl_gl_helper::attrib* attrib, const player* _player, std::uint32_t sign) const noexcept;
    void render_sign(const sdl_gl_helper::attrib* attrib, player* _player, std::uint32_t sign) const noexcept;
    void render_players(const sdl_gl_helper::attrib* attrib, player* _player) const noexcept;
    void render_wireframe(const sdl_gl_helper::attrib* attrib, const player* _player) const noexcept;
    void render_crosshairs(const sdl_gl_helper::attrib* attrib) const noexcept;
    void render_item(const sdl_gl_helper::attrib* attrib, std::uint32_t texture) const noexcept;
    void render_text(const sdl_gl_helper::attrib* attrib, std::uint32_t font, int justify, float x, float y, float n, std::string_view text) const noexcept;

    enum class Layer
    {
        BACKGROUND = 0,
        FOREGROUND = 1,
        LAYER_COUNT = 2
    };

    static constexpr auto FORCE_DUE_TO_GRAVITY = -9.8f;

    const sdl_gl_helper* m_sdl;

    font_manager& m_fonts;
    shader_manager& m_shaders;
    texture_manager& m_textures;

    static constexpr auto MAX_CHUNKS = 8192;
    std::array<std::array<scene_node*, MAX_CHUNKS>, static_cast<std::size_t>(Layer::LAYER_COUNT)> m_scene_layers;

    std::size_t m_next_chunk_slot;

    command_queue m_command_queue;
    player* m_player;

    model m_model;

    std::vector<std::unique_ptr<worker>> m_workers;
};

#endif // WORLD_H
