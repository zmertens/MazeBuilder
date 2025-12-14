#ifndef WORLD_H
#define WORLD_H

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "command_queue.h"

#include "map.h"
#include "resource_identifiers.h"
#include "sdl_gl_helper.h"
#include "sign.h"

struct worker;
struct worker_item;
union SDL_Event;

std::string gl_error_checker(const char* file, int line) noexcept;

#define CHECK_GL_ERR() gl_error_checker(__FILE__, __LINE__)

using world_func = std::function<void(int, int, int, int, Map*)>;

class attrib;
class command_queue;
class sdl_gl_helper;

namespace mazes {
    class randomizer;
}

class world final {
    friend class player;
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

    static void create_world(int p, int q, const world_func& func, Map *m, int chunk_size) noexcept;

    // Light manipulation (public for player actions)
    void on_light() const noexcept;

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

    struct model {
        bool is_ortho;
        float fov;
        int day_length;
        int start_time;
        int start_ticks;
    };

    bool worker_run(worker* w) const noexcept;
    void init_worker_threads() noexcept;
    void cleanup_worker_threads() noexcept;

    [[nodiscard]] static int chunked(float x) noexcept;

    [[nodiscard]] double get_time() const noexcept;
    [[nodiscard]] float time_of_day() const noexcept;
    [[nodiscard]] float get_daylight() const noexcept;

    [[nodiscard]] std::optional<scene_node*> find_chunk(int p, int q) const noexcept;
    static int chunk_distance(const scene_node* chunk, int p, int q) noexcept;
    int chunk_visible(float planes[6][4], int p, int q, int miny, int maxy) const noexcept;

    [[nodiscard]] int highest_block(float x, float z) const noexcept;
    static int _hit_test(const Map* map, float max_distance, int previous,
        float x, float y, float z, float vx, float vy, float vz, int* hx, int* hy, int* hz) noexcept;
    int hit_test(int previous, float x, float y, float z, float rx, float ry, int* bx, int* by, int* bz) const noexcept;
    int hit_test_face(int* x, int* y, int* z, int* face) const noexcept;
    int collide(int height, float* x, float* y, float* z) const noexcept;
    [[nodiscard]] static bool player_intersects_block(int height, float x, float y, float z, int hx, int hy, int hz) noexcept;

    bool has_lights(const scene_node* chunk) const noexcept;

    void dirty_chunk(scene_node* chunk) const noexcept;
    // Process dirty chunks on worker threads
    void update_dirty_chunks_async() const noexcept;

    static void occlusion(char neighbors[27], char lights[27], float shades[27], float ao[6][4], float light[6][4]) noexcept;
    static void light_fill(char* opaque, char* light, int x, int y, int z, int w, int force) noexcept;

    static void compute_chunk(worker_item* item) noexcept;

    static void generate_chunk(scene_node* chunk, const worker_item* item) noexcept;
    void gen_chunk_buffer(scene_node* chunk) const noexcept;

    static void map_set_func(int x, int y, int z, int w, Map* m) noexcept;

    static void load_chunk(const worker_item* item) noexcept;
    void init_chunk(scene_node* chunk, int p, int q) noexcept;
    void create_chunk(scene_node* chunk, int p, int q) noexcept;
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
    static void record_block(int x, int y, int z, int w) noexcept;
    [[nodiscard]] int get_block(int x, int y, int z) const noexcept;
    void builder_block(int x, int y, int z, int w) const noexcept;

    int render_chunks(const sdl_gl_helper::attrib* attrib, uint32_t texture) const noexcept;
    void render_signs(const sdl_gl_helper::attrib* attrib, std::uint32_t sign) const noexcept;
    void render_sign(const sdl_gl_helper::attrib* attrib, std::uint32_t sign) const noexcept;
    void render_sky(const sdl_gl_helper::attrib* attrib, std::uint32_t buffer,
        std::uint32_t sign) const noexcept;
    void render_players(const sdl_gl_helper::attrib* attrib) const noexcept;
    void render_wireframe(const sdl_gl_helper::attrib* attrib) const noexcept;
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

    std::uint32_t m_sky_buffer;
};

#endif // WORLD_H
