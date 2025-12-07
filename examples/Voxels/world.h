#ifndef WORLD_H
#define WORLD_H

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
#include "sign.h"
#include "glad/glad.h"

#define KEY_FORWARD SDL_SCANCODE_W
#define KEY_BACKWARD SDL_SCANCODE_S
#define KEY_LEFT SDL_SCANCODE_A
#define KEY_RIGHT SDL_SCANCODE_D
#define KEY_JUMP SDL_SCANCODE_SPACE
#define KEY_FLY SDL_SCANCODE_TAB
#define KEY_ITEM_NEXT SDL_SCANCODE_E
#define KEY_ITEM_PREV SDL_SCANCODE_R
#define KEY_ZOOM SDL_SCANCODE_LSHIFT
#define KEY_ORTHO SDL_SCANCODE_F
#define KEY_TAG SDL_SCANCODE_T

// World configs
#define SCROLL_THRESHOLD 0.1
#define MAX_DB_PATH_LEN 64
#define USE_CACHE true
#define DAY_LENGTH 600
#define INVERT_MOUSE 0
#define MAX_TEXT_LENGTH 256

// Advanced options
#define CREATE_CHUNK_RADIUS 10
#define RENDER_CHUNK_RADIUS 20
#define BUILD_CHUNK_SIZE 32
#define RENDER_SIGN_RADIUS 4
#define DELETE_CHUNK_RADIUS 14
#define COMMIT_INTERVAL 7
#define MAX_CHUNKS 8192
#define MAX_PLAYERS 1
#define NUM_WORKERS 4

#define WORKER_IDLE 0
#define WORKER_BUSY 1
#define WORKER_DONE 2

union SDL_Event;

std::string gl_error_checker(const char* file, int line) noexcept;

#define CHECK_GL_ERR() gl_error_checker(__FILE__, __LINE__)

using world_func = std::function<void(int, int, int, int, Map*)>;

class command_queue;
class player;
struct SDL_Window;

namespace mazes {
    class randomizer;
}

class world final {
public:
    explicit world(SDL_Window* window,
        font_manager& fonts,
        player* p,
        shader_manager& shaders,
        texture_manager& textures);

    ~world();

    void init() noexcept;

    void update(float dt, mazes::randomizer& rng) noexcept;

    void draw() const noexcept;

    command_queue& get_command_queue() noexcept;

    // Destroy the world
    void destroy_world();

    void handle_event(SDL_Event& event) noexcept;

    static void create_world(int p, int q, world_func func, Map *m, int chunk_size) noexcept;

private:
    // Build the scene (initialize scene graph and layers)
    void build_scene();

#define MAX_SIGN_LENGTH 16

    struct Chunk {
        Map map;
        Map lights;
        SignList signs;
        int p;
        int q;
        int faces;
        int sign_faces;
        int dirty;
        int miny;
        int maxy;
        std::uint32_t buffer;
        std::uint32_t sign_buffer;
    };

    struct WorkerItem {
        int p{};
        int q{};
        int load{};
        Map* block_maps[3][3]{};
        Map* light_maps[3][3]{};
        int miny{};
        int maxy{};
        int faces{};
        float* data{};
        WorkerItem() {
        }
    };

    typedef struct {
        int index;
        int state;
        std::thread thrd;
        std::mutex mtx;
        std::condition_variable cnd;
        WorkerItem item;
        bool should_stop;
    } Worker;

    typedef struct {
        int x;
        int y;
        int z;
        int w;
    } Block;

    typedef struct {
        float x;
        float y;
        float z;
        float rx;
        float ry;
        float t;
    } State;

    typedef struct {
        std::uint32_t program;
        std::uint32_t position;
        std::uint32_t normal;
        std::uint32_t uv;
        std::uint32_t matrix;
        std::uint32_t sampler;
        std::uint32_t camera;
        std::uint32_t timer;
        std::uint32_t extra1;
        std::uint32_t extra2;
        std::uint32_t extra3;
        std::uint32_t extra4;
    } Attrib;

    typedef struct {
        std::vector<std::unique_ptr<Worker>> workers;
        Chunk chunks[MAX_CHUNKS];
        int chunk_count;
        int create_radius;
        int render_radius;
        int delete_radius;
        int sign_radius;
        int player_count;
        int voxel_scene_w;
        int voxel_scene_h;
        bool flying;
        int item_index;
        int scale;
        bool is_ortho;
        float fov;
        char db_path[MAX_DB_PATH_LEN];
        int day_length;
        int start_time;
        int start_ticks;
        Block block0;
        Block block1;
        Block copy0;
        Block copy1;
    } Model;

    bool worker_run(void* arg) noexcept;
    void init_worker_threads() noexcept;
    void cleanup_worker_threads() noexcept;

    void del_buffer(std::uint32_t buffer) const noexcept;
    std::uint32_t gen_buffer(std::size_t size, const float* data) const noexcept;
    [[nodiscard]] float* malloc_faces(std::size_t components, std::size_t faces) const noexcept;
    std::uint32_t gen_faces(std::size_t components, std::size_t faces, const float* data) const noexcept;

    [[nodiscard]] int chunked(float x) const noexcept;

    [[nodiscard]] double get_time() const noexcept;
    [[nodiscard]] float time_of_day() const noexcept;
    [[nodiscard]] float get_daylight() const noexcept;

    void compute_sight_vector(float rx, float ry, float* vx, float* vy, float* vz) const noexcept;
    void compute_motion_vector(int flying, int sz, int sx, float rx, float ry,
        float* vx, float* vy, float* vz) const noexcept;
    [[nodiscard]] std::uint32_t gen_crosshair_buffer() const noexcept;
    [[nodiscard]] std::uint32_t gen_wireframe_buffer(float x, float y, float z, float n) const noexcept;
    [[nodiscard]] std::uint32_t gen_cube_buffer(float x, float y, float z, float n, int w) const noexcept;
    [[nodiscard]] std::uint32_t gen_plant_buffer(float x, float y, float z, float n, int w) const noexcept;
    [[nodiscard]] std::uint32_t gen_player_buffer(float x, float y, float z, float rx, float ry) const noexcept;
    [[nodiscard]] std::uint32_t gen_text_buffer(float x, float y, float n, std::string_view text) const noexcept;

    void draw_triangles_3d_ao(const Attrib* attrib, std::uint32_t buffer, int count) const noexcept;
    void draw_triangles_3d_text(const Attrib* attrib, std::uint32_t buffer, int count) const noexcept;
    void draw_triangles_3d(const Attrib* attrib, std::uint32_t buffer, int count) const noexcept;
    void draw_triangles_2d(const Attrib* attrib, std::uint32_t buffer, std::size_t count) const noexcept;
    void draw_lines(const Attrib* attrib, std::uint32_t buffer, int components, int count) const noexcept;
    void draw_chunk(const Attrib* attrib, const Chunk* chunk) const noexcept;
    void draw_item(const Attrib* attrib, std::uint32_t buffer, int count) const noexcept;
    void draw_text(const Attrib* attrib, std::uint32_t buffer, std::size_t length) const noexcept;
    void draw_signs(const Attrib* attrib, const Chunk* chunk) const noexcept;
    void draw_sign(const Attrib* attrib, GLuint buffer, int length) const noexcept;
    void draw_cube(const Attrib* attrib, GLuint buffer) const noexcept;
    void draw_plant(const Attrib* attrib, std::uint32_t buffer) const noexcept;
    void draw_player(const Attrib* attrib, const player* player) const noexcept;

    [[nodiscard]] const player* find_player(int id) const noexcept;
    void delete_all_players() noexcept;

    [[nodiscard]] std::optional<Chunk*> find_chunk(int p, int q) const noexcept;
    static int chunk_distance(const Chunk* chunk, int p, int q) noexcept;
    int chunk_visible(float planes[6][4], int p, int q, int miny, int maxy) const noexcept;

    [[nodiscard]] int highest_block(float x, float z) const noexcept;
    static int _hit_test(const Map* map, float max_distance, int previous,
        float x, float y, float z, float vx, float vy, float vz, int* hx, int* hy, int* hz) noexcept;
    int hit_test(int previous, float x, float y, float z, float rx, float ry, int* bx, int* by, int* bz) const noexcept;
    int hit_test_face(player* _player, int* x, int* y, int* z, int* face) const noexcept;
    int collide(int height, float* x, float* y, float* z) const noexcept;
    [[nodiscard]] int player_intersects_block(int height, float x, float y, float z, int hx, int hy, int hz) const noexcept;

    int _gen_sign_buffer(float* data, float x, float y, float z, int face, std::string_view text) const noexcept;
    void gen_sign_buffer(Chunk* chunk) const noexcept;

    int has_lights(Chunk* chunk) const noexcept;

    void dirty_chunk(Chunk* chunk) const noexcept;

    void occlusion(char neighbors[27], char lights[27], float shades[27], float ao[6][4], float light[6][4]) const noexcept;
    void light_fill(char* opaque, char* light, int x, int y, int z, int w, int force) const noexcept;

    void compute_chunk(WorkerItem* item) const noexcept;

    void generate_chunk(Chunk* chunk, WorkerItem* item) const noexcept;
    void gen_chunk_buffer(Chunk* chunk) const noexcept;

    static void map_set_func(int x, int y, int z, int w, Map* m) noexcept;

    void load_chunk(WorkerItem* item) const noexcept;
    void init_chunk(Chunk* chunk, int p, int q) const noexcept;
    void create_chunk(Chunk* chunk, int p, int q) const noexcept;
    void delete_chunks() noexcept;
    void delete_all_chunks() noexcept;
    void force_chunks(player* player) noexcept;
    void check_workers() noexcept;
    void ensure_chunks_worker(player* _player, Worker* worker) noexcept;
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

    int render_chunks(const Attrib* attrib, player* _player, std::uint32_t texture) const noexcept;
    void render_signs(const Attrib* attrib, player* _player, std::uint32_t sign) const noexcept;
    void render_sign(const Attrib* attrib, player* _player, std::uint32_t sign) const noexcept;
    void render_players(const Attrib* attrib, player* _player) const noexcept;
    void render_wireframe(const Attrib* attrib, player* _player) const noexcept;
    void render_crosshairs(const Attrib* attrib) const noexcept;
    void render_item(const Attrib* attrib, std::uint32_t texture) const noexcept;
    void render_text(const Attrib* attrib, std::uint32_t font, int justify, float x, float y, float n, std::string_view text) const noexcept;

    void on_light() noexcept;
    void on_left_click() noexcept;
    void on_right_click() noexcept;
    void on_middle_click() noexcept;

    enum class Layer
    {
        PARALLAX_BACK = 0,
        PARALLAX_MID = 1,
        PARALLAX_FORE = 2,
        BACKGROUND = 3,
        FOREGROUND = 4,
        LAYER_COUNT = 5
    };

    static constexpr auto FORCE_DUE_TO_GRAVITY = -9.8f;

    SDL_Window* m_window;

    font_manager& m_fonts;
    shader_manager& m_shaders;
    texture_manager& m_textures;
    // SceneNode mSceneGraph;
    // std::array<SceneNode*, static_cast<std::size_t>(Layer::LAYER_COUNT)> mSceneLayers;

    command_queue m_command_queue;
    player* m_player;

    Attrib m_block_attrib;
    Attrib m_line_attrib;
    Attrib m_text_attrib;
    Attrib m_sky_attrib;

    Model m_model;
};

#endif // WORLD_H
