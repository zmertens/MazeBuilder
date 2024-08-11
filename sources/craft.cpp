/*
 * Craft engine builds voxels as chunks and can run maze-generating algorithms
 * Generated mazes are stored in-memory and in an offline database
 * Vertex and indice data is stored in buffers and rendered using OpenGL
 * Supports writing to Wavefront OBJ files
 * Interfaces with Emscripten to provide data in JSON-format to web applications
 * 
 * 
 * Originally written in C99, ported to C++17
*/

#include "craft.h"

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_opengl3.h>
#include "nunito_sans.h"

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#include <emscripten_local/emscripten_mainloop_stub.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>

#define SDL_FUNCTION_POINTER_IS_VOID_POINTER

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <string_view>
#include <algorithm>
#include <iostream>
#include <thread>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <random>
#include <utility>
#include <tuple>

#include <noise/noise.h>

#include "util.h"
#include "world.h"
#include "cube.h"
#include "db.h"
#include "item.h"
#include "matrix.h"

#include "maze_types_enum.h"
#include "maze_factory.h"
#include "maze_thread_safe.h"
#include "grid.h"
#include "cell.h"
#include "writer.h"

// basic configurations
#define KEY_FORWARD SDL_SCANCODE_W
#define KEY_BACKWARD SDL_SCANCODE_S
#define KEY_LEFT SDL_SCANCODE_A
#define KEY_RIGHT SDL_SCANCODE_D
#define KEY_JUMP SDL_SCANCODE_SPACE
#define KEY_FLY SDL_SCANCODE_TAB
#define KEY_OBSERVE SDL_SCANCODE_O
#define KEY_OBSERVE_INSET SDL_SCANCODE_P
#define KEY_ITEM_NEXT SDL_SCANCODE_E
#define KEY_ITEM_PREV SDL_SCANCODE_R
#define KEY_ZOOM SDL_SCANCODE_LSHIFT
#define KEY_ORTHO SDL_SCANCODE_F
#define KEY_CHAT SDL_SCANCODE_T
#define KEY_COMMAND SDL_SCANCODE_SLASH
#define KEY_SIGN SDL_SCANCODE_GRAVE

#define INIT_WINDOW_WIDTH 1024
#define INIT_WINDOW_HEIGHT 768
#define SCROLL_THRESHOLD 0.1
#define MAX_MESSAGES 4
static constexpr auto DB_PATH = "craft.db";
static constexpr auto USE_CACHE = true;
#define DAY_LENGTH 600
#define INVERT_MOUSE 0

// rendering options
#define SHOW_INFO_TEXT 1
#define SHOW_CHAT_TEXT 1
#define SHOW_PLAYER_NAMES 1

#define CRAFT_KEY_SIGN '`'

// advanced parameters
#define CREATE_CHUNK_RADIUS 10
#define RENDER_CHUNK_RADIUS 20
#define RENDER_SIGN_RADIUS 4
#define DELETE_CHUNK_RADIUS 14
#define COMMIT_INTERVAL 5

#define MAX_CHUNKS 8192
#define MAX_PLAYERS 1
#define NUM_WORKERS 4
static constexpr auto MAX_TEXT_LENGTH = 256;
#define MAX_NAME_LENGTH 32
#define MAX_PATH_LENGTH 256
#define MAX_ADDR_LENGTH 256

#define ALIGN_LEFT 0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT 2

#define WORKER_IDLE 0
#define WORKER_BUSY 1
#define WORKER_DONE 2

using namespace mazes;
using namespace std;

struct craft::craft_impl {
    // Builder pattern for the Gui
    class Gui {
    public:
        bool fullscreen;
        bool vsync;
        bool color_mode_dark;
        bool capture_mouse;
        int chunk_size;
        bool show_trees;
        bool show_plants;
        bool show_clouds;
        bool show_lights;
        bool show_items;
        bool show_wireframes;
        bool show_crosshairs;
        char outfile[64];
        int seed;
        int maze_width;
        int maze_height;
        int maze_length;
        std::string maze_algo;
        std::string maze_json;

        Gui() : fullscreen(false), vsync(true), color_mode_dark(false),
            capture_mouse(false), chunk_size(8), show_trees(true),
            show_plants(true), show_clouds(true), show_lights(true),
            show_items(true), show_wireframes(true), show_crosshairs(true),
            outfile(".obj"), seed(101), maze_width(100), maze_height(10), maze_length(100),
            maze_algo("binary_tree"), maze_json("") {
        
        }

        void reset_outfile() {
            for (auto i = 0; i < IM_ARRAYSIZE(outfile); ++i) {
                outfile[i] = '\0';
            }
            outfile[0] = '.';
            outfile[1] = 'o';
            outfile[2] = 'b';
            outfile[3] = 'j';
        }
    }; // class
    
    class GuiBuilder {
        Gui gui;
    public:
        GuiBuilder& fullscreen(bool fullscreen) {
            gui.fullscreen = fullscreen;
            return *this;
        }

        GuiBuilder& vsync(bool vsync) {
            gui.vsync = vsync;
            return *this;
        }

        GuiBuilder& color_mode_dark(bool value) {
            gui.color_mode_dark = value;
            return *this;
        }

        GuiBuilder& capture_mouse(bool value) {
            gui.capture_mouse = value;
            return *this;
        }

        GuiBuilder& chunk_size(int size) {
            gui.chunk_size = size;
            return *this;
        }

        GuiBuilder& show_trees(bool value) {
            gui.show_trees = value;
            return *this;
        }

        GuiBuilder& show_plants(bool value) {
            gui.show_plants = value;
            return *this;
        }

        GuiBuilder& show_clouds(bool value) {
            gui.show_clouds = value;
            return *this;
        }

        GuiBuilder& show_lights(bool value) {
            gui.show_lights = value;
            return *this;
        }

        GuiBuilder& show_items(bool value) {
            gui.show_items = value;
            return *this;
        }

        GuiBuilder& show_wireframes(bool value) {
            gui.show_wireframes = value;
            return *this;
        }

        GuiBuilder& show_crosshairs(bool value) {
            gui.show_crosshairs = value;
            return *this;
        }

        Gui build() const {
            return gui;
        }
    }; // class

    struct ProgressTracker {
        std::atomic<std::chrono::steady_clock::time_point> start_time;
        std::atomic<std::chrono::steady_clock::time_point> end_time;
        
        void start() {
            start_time.store(std::chrono::steady_clock::now());
        }

        void stop() {
            end_time.store(std::chrono::steady_clock::now());
        }

        double get_duration_in_seconds() {
            return std::chrono::duration<double>(end_time.load() - start_time.load()).count();
        }

        double get_duration_in_ms() {
            return std::chrono::duration<double>(end_time.load() - start_time.load()).count() * 1000.0;
        }
    };

    typedef struct {
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
        GLuint buffer;
        GLuint sign_buffer;
    } Chunk;

    typedef struct {
        int p;
        int q;
        int load;
        Map *block_maps[3][3];
        Map *light_maps[3][3];
        int miny;
        int maxy;
        int faces;
        GLfloat *data;
    } WorkerItem;

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
        int id;
        char name[MAX_NAME_LENGTH];
        State state;
        State state1;
        State state2;
        GLuint buffer;
    } Player;

    typedef struct {
        GLuint program;
        GLuint position;
        GLuint normal;
        GLuint uv;
        GLuint matrix;
        GLuint sampler;
        GLuint camera;
        GLuint timer;
        GLuint extra1;
        GLuint extra2;
        GLuint extra3;
        GLuint extra4;
    } Attrib;

    typedef struct {
        SDL_Window *window;
        SDL_GLContext context;
        std::vector<std::unique_ptr<Worker>> workers;
        Chunk chunks[MAX_CHUNKS];
        int chunk_count;
        int create_radius;
        int render_radius;
        int delete_radius;
        int sign_radius;
        Player players[MAX_PLAYERS];
        int player_count;
        int typing;
        char typing_buffer[MAX_TEXT_LENGTH];
        size_t text_len;
        int message_index;
        char messages[MAX_MESSAGES][MAX_TEXT_LENGTH];
        int width;
        int height;
        int observe1;
        int observe2;
        int flying;
        int item_index;
        int scale;
        bool is_ortho;
        float fov;
        int suppress_char;
        int mode_changed;
        char db_path[MAX_PATH_LENGTH];
        char server_addr[MAX_ADDR_LENGTH];
        int server_port;
        int day_length;
        int time_changed;
        int start_time;
        int start_ticks;
        Block block0;
        Block block1;
        Block copy0;
        Block copy1;
    } Model;

    // Note: These are public members
    const std::string& m_window_name;
    const std::string& m_version;
    const std::string& m_help;

    unique_ptr<Model> m_model;
    unique_ptr<maze_thread_safe> m_maze;
    unique_ptr<Gui> m_gui;

    craft_impl(const std::string& window_name, const std::string& version, const std::string& help)
        : m_window_name{ window_name }
        , m_version{ version }
        , m_help{help}
        , m_model{make_unique<Model>()}
        // Construct maze in run loop
        , m_maze()
        , m_gui{make_unique<Gui>()} {
        m_model->width = INIT_WINDOW_WIDTH;
        m_model->height = INIT_WINDOW_HEIGHT;
        m_model->scale = 1;
        m_model->day_length = DAY_LENGTH;
    }

    int worker_run(void *arg) {
        Worker* worker = reinterpret_cast<Worker*>(arg);
        while (1) {
            while (worker->state != WORKER_BUSY && !worker->should_stop) {
                unique_lock<mutex> u_lck(worker->mtx);
                worker->cnd.wait(u_lck);
            }
            if (worker->should_stop) {
				break;
			}

            WorkerItem *worker_item = &worker->item;
            if (worker_item->load) {
                this->load_chunk(worker_item);
            }
            
            this->compute_chunk(worker_item);

            worker->mtx.lock();
            worker->state = WORKER_DONE;
            worker->mtx.unlock();
        }
        return 0;
    } // worker_run

    void init_worker_threads() {
        this->m_model->workers.reserve(NUM_WORKERS);
        for (int i = 0; i < NUM_WORKERS; i++) {
            auto worker = make_unique<Worker>();
            worker->index = i;
            worker->state = WORKER_IDLE;
            worker->thrd = thread([this](void* arg) { this->worker_run(arg); }, worker.get());
            this->m_model->workers.emplace_back(std::move(worker));
        }
    }

    /**
     * Cleanup the worker threads
     */
    void cleanup_worker_threads() {
        // signal all worker threads to stop
        for (auto&& w : this->m_model->workers) {
            w->mtx.lock();
            w->should_stop = true;
            w->cnd.notify_one();
            w->mtx.unlock();
        }
        // Wait for threads to join
        for (auto&& w : this->m_model->workers) {
            // Wait for the thread to complete its execution
            w->thrd.join();
#if defined(MAZE_DEBUG)
            SDL_Log("Worker thread %d finished!", w->index);
#endif
        }
        // Clear the vector after all threads have been joined
        this->m_model->workers.clear();
    }

    void del_buffer(GLuint buffer) const {
        glDeleteBuffers(1, &buffer);
    }

    GLuint gen_buffer(GLsizei size, GLfloat *data) const {
        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return buffer;
    }

    GLfloat *malloc_faces(std::size_t components, std::size_t faces) const {
        return (GLfloat*) SDL_malloc(sizeof(GLfloat) * 6 * components * faces);
    }

    /**
     * Generate a buffer for faces - data is not freed here
     */
    GLuint gen_faces(GLsizei components, GLsizei faces, GLfloat *data) const {
        GLuint buffer = this->gen_buffer(sizeof(GLfloat) * 6 * components * faces, data);
        return buffer;
    }

    int chunked(float x) const {
        return static_cast<int>((SDL_roundf(x) / this->m_gui->chunk_size));
    }

    double get_time() const {
    	return (SDL_GetTicks() + (double) this->m_model->start_time - (double) this->m_model->start_ticks) / 1000.0;
    }

    float time_of_day() const {
        if (this->m_model->day_length <= 0) {
            return 0.5f;
        }
        float t;
        t = static_cast<float>(get_time());
        t = t / static_cast<float>(this->m_model->day_length);
        t = static_cast<float>(t - static_cast<int>(t));
        return t;
    }

    float get_daylight() const {
        float timer = time_of_day();
        if (timer < 0.5) {
            float t = (timer - 0.25f) * 100.f;
            return 1 / (1 + SDL_powf(2.f, -t));
        }
        else {
            float t = (timer - 0.85f) * 100.f;
            return 1.f - 1.f / (1.f + SDL_powf(2.f, -t));
        }
    }

    int get_scale_factor() const {
        int window_width, window_height;
        int buffer_width, buffer_height;
        SDL_GetWindowSize(this->m_model->window, &window_width, &window_height);
        SDL_GetWindowSizeInPixels(this->m_model->window, &buffer_width, &buffer_height);
        int result = buffer_width / window_width;
        result = SDL_max(1, result);
        result = SDL_min(2, result);
        return result;
    }

    void get_sight_vector(float rx, float ry, float *vx, float *vy, float *vz) const {
        float m = SDL_cosf(ry);
        *vx = SDL_cosf(rx - static_cast<float>(RADIANS(90))) * m;
        *vy = SDL_sinf(ry);
        *vz = SDL_sinf(rx - static_cast<float>(RADIANS(90))) * m;
    }

    void get_motion_vector(int flying, int sz, int sx, float rx, float ry,
        float *vx, float *vy, float *vz) const {
        *vx = 0; *vy = 0; *vz = 0;
        if (!sz && !sx) {
            return;
        }
        float strafe = SDL_atan2f(static_cast<float>(sz), static_cast<float>(sx));
        if (flying) {
            float m = SDL_cosf(ry);
            float y = SDL_sinf(ry);
            if (sx) {
                if (!sz) {
                    y = 0;
                }
                m = 1;
            }
            if (sz > 0) {
                y = -y;
            }
            *vx = SDL_cosf(rx + strafe) * m;
            *vy = y;
            *vz = SDL_sinf(rx + strafe) * m;
        }
        else {
            *vx = SDL_cosf(rx + strafe);
            *vy = 0;
            *vz = SDL_sinf(rx + strafe);
        }
    }

    GLuint gen_crosshair_buffer() {
        float x = static_cast<float>(this->m_model->width) / 2.0f;
        float y = static_cast<float>(this->m_model->height) / 2.0f;
        float p = 10.f * static_cast<float>(this->m_model->scale);
        float data[] = {
            x, y - p, x, y + p,
            x - p, y, x + p, y
        };
        return this->gen_buffer(sizeof(data), data);
    }

    GLuint gen_wireframe_buffer(float x, float y, float z, float n) {
        float data[72];
        // cube.h -> make_cube_wireframe
        make_cube_wireframe(data, x, y, z, n);
        return this->gen_buffer(sizeof(data), data);
    }

    GLuint gen_sky_buffer() {
        float data[12288];
        // cube.h -> make_sphere
        make_sphere(data, 1, 3);
        return this->gen_buffer(sizeof(data), data);
    }

    GLuint gen_cube_buffer(float x, float y, float z, float n, int w) {
        GLfloat *data = malloc_faces(10, 6);
        float ao[6][4] = {0};
        float light[6][4] = {
            {0.5, 0.5, 0.5, 0.5},
            {0.5, 0.5, 0.5, 0.5},
            {0.5, 0.5, 0.5, 0.5},
            {0.5, 0.5, 0.5, 0.5},
            {0.5, 0.5, 0.5, 0.5},
            {0.5, 0.5, 0.5, 0.5}
        };
        make_cube(data, ao, light, 1, 1, 1, 1, 1, 1, x, y, z, n, w);
        return gen_faces(10, 6, data);
    }

    GLuint gen_plant_buffer(float x, float y, float z, float n, int w) {
        GLfloat *data = malloc_faces(10, 4);
        float ao = 0;
        float light = 1;
        make_plant(data, ao, light, x, y, z, n, w, 45);
        return gen_faces(10, 4, data);
    }

    GLuint gen_player_buffer(float x, float y, float z, float rx, float ry) {
        GLfloat *data = malloc_faces(10, 6);
        make_player(data, x, y, z, rx, ry);
        return gen_faces(10, 6, data);
    }

    GLuint gen_text_buffer(float x, float y, float n, char *text) {
        GLsizei length = static_cast<GLsizei>(SDL_strlen(text));
        GLfloat *data = malloc_faces(4, length);
        for (int i = 0; i < length; i++) {
            make_character(data + i * 24, x, y, n / 2, n, text[i]);
            x += n;
        }
        return gen_faces(4, length, data);
    }

    void draw_triangles_3d_ao(Attrib *attrib, GLuint buffer, int count) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glEnableVertexAttribArray(attrib->position);
        glEnableVertexAttribArray(attrib->normal);
        glEnableVertexAttribArray(attrib->uv);
        glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 10, 0);
        glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 10, (GLvoid *)(sizeof(GLfloat) * 3));
        glVertexAttribPointer(attrib->uv, 4, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 10, (GLvoid *)(sizeof(GLfloat) * 6));
        glDrawArrays(GL_TRIANGLES, 0, count);
        glDisableVertexAttribArray(attrib->position);
        glDisableVertexAttribArray(attrib->normal);
        glDisableVertexAttribArray(attrib->uv);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_triangles_3d_text(Attrib *attrib, GLuint buffer, int count) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glEnableVertexAttribArray(attrib->position);
        glEnableVertexAttribArray(attrib->uv);
        glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 5, 0);
        glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 5, (GLvoid *)(sizeof(GLfloat) * 3));
        glDrawArrays(GL_TRIANGLES, 0, count);
        glDisableVertexAttribArray(attrib->position);
        glDisableVertexAttribArray(attrib->uv);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // Sky Attrib doesn't use normals and the GLSL compiler may optimize it out
    void draw_triangles_3d(Attrib *attrib, GLuint buffer, int count) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);

        glEnableVertexAttribArray(attrib->position);

        glEnableVertexAttribArray(attrib->normal);
        glEnableVertexAttribArray(attrib->uv);

        glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, 0);
        glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (GLvoid *)(sizeof(GLfloat) * 3));
        glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (GLvoid *)(sizeof(GLfloat) * 6));
        
        glDrawArrays(GL_TRIANGLES, 0, count);

        glDisableVertexAttribArray(attrib->position);
        glDisableVertexAttribArray(attrib->normal);
        glDisableVertexAttribArray(attrib->uv);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_triangles_2d(Attrib *attrib, GLuint buffer, GLsizei count) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glEnableVertexAttribArray(attrib->position);
        glEnableVertexAttribArray(attrib->uv);
        glVertexAttribPointer(attrib->position, 2, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 4, 0);
        glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 4, (GLvoid *)(sizeof(GLfloat) * 2));
        glDrawArrays(GL_TRIANGLES, 0, count);
        glDisableVertexAttribArray(attrib->position);
        glDisableVertexAttribArray(attrib->uv);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_lines(Attrib *attrib, GLuint buffer, int components, int count) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glEnableVertexAttribArray(attrib->position);
        glVertexAttribPointer(
            attrib->position, components, GL_FLOAT, GL_FALSE, 0, 0);
        glDrawArrays(GL_LINES, 0, count);
        glDisableVertexAttribArray(attrib->position);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_chunk(Attrib *attrib, Chunk *chunk) {
        draw_triangles_3d_ao(attrib, chunk->buffer, chunk->faces * 6);
    }

    void draw_item(Attrib *attrib, GLuint buffer, int count) {
        draw_triangles_3d_ao(attrib, buffer, count);
    }

    void draw_text(Attrib *attrib, GLuint buffer, GLsizei length) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        draw_triangles_2d(attrib, buffer, length * 6);
        glDisable(GL_BLEND);
    }

    void draw_signs(Attrib *attrib, Chunk *chunk) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-8, -1024);
        draw_triangles_3d_text(attrib, chunk->sign_buffer, chunk->sign_faces * 6);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    void draw_sign(Attrib *attrib, GLuint buffer, int length) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-8, -1024);
        draw_triangles_3d_text(attrib, buffer, length * 6);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    void draw_cube(Attrib *attrib, GLuint buffer) {
        draw_item(attrib, buffer, 36);
    }

    void draw_plant(Attrib *attrib, GLuint buffer) {
        draw_item(attrib, buffer, 24);
    }

    void draw_player(Attrib *attrib, Player *player) {
        draw_cube(attrib, player->buffer);
    }

    Player *find_player(int id) {
        for (int i = 0; i < this->m_model->player_count; i++) {
            Player *player = this->m_model->players + i;
            if (player->id == id) {
                return player;
            }
        }
        return 0;
    }

    void update_player(Player *player, float x, float y, float z, float rx, float ry, int interpolate)
    {
        if (interpolate) {
            State *s1 = &player->state1;
            State *s2 = &player->state2;
            SDL_memcpy(s1, s2, sizeof(State));
            s2->x = x; s2->y = y; s2->z = z; s2->rx = rx; s2->ry = ry;
            s2->t = static_cast<float>(get_time());
            if (s2->rx - s1->rx > static_cast<float>(PI)) {
                s1->rx += static_cast<float>(2 * PI);
            }
            if (s1->rx - s2->rx > static_cast<float>(PI)) {
                s1->rx -= static_cast<float>(2 * PI);
            }
        }
        else {
            State *s = &player->state;
            s->x = x; s->y = y; s->z = z; s->rx = rx; s->ry = ry;
            del_buffer(player->buffer);
            player->buffer = gen_player_buffer(s->x, s->y, s->z, s->rx, s->ry);
        }
    }

    void interpolate_player(Player *player) {
        State *s1 = &player->state1;
        State *s2 = &player->state2;
        float t1 = static_cast<float>(s2->t - s1->t);
        float t2 = static_cast<float>(this->get_time()) - s2->t;
        t1 = SDL_min(t1, 1.f);
        t1 = SDL_max(t1, 0.1f);
        float p = SDL_min(t2 / t1, 1.f);
        this->update_player(
            player,
            s1->x + (s2->x - s1->x) * p,
            s1->y + (s2->y - s1->y) * p,
            s1->z + (s2->z - s1->z) * p,
            s1->rx + (s2->rx - s1->rx) * p,
            s1->ry + (s2->ry - s1->ry) * p,
            0);
    }

    void delete_player(int id) {
        Player *player = this->find_player(id);
        if (!player) {
            return;
        }
        int count = this->m_model->player_count;
        this->del_buffer(player->buffer);
        Player *other = this->m_model->players + (--count);
        SDL_memcpy(player, other, sizeof(Player));
        this->m_model->player_count = count;
    }

    void delete_all_players() {
        for (int i = 0; i < this->m_model->player_count; i++) {
            Player *player = this->m_model->players + i;
            this->del_buffer(player->buffer);
        }
        this->m_model->player_count = 0;
    }

    float player_player_distance(Player *p1, Player *p2) {
        State *s1 = &p1->state;
        State *s2 = &p2->state;
        float x = s2->x - s1->x;
        float y = s2->y - s1->y;
        float z = s2->z - s1->z;
        return SDL_sqrtf(x * x + y * y + z * z);
    }

    float player_crosshair_distance(Player *p1, Player *p2) {
        State *s1 = &p1->state;
        State *s2 = &p2->state;
        float d = this->player_player_distance(p1, p2);
        float vx, vy, vz;
        this->get_sight_vector(s1->rx, s1->ry, &vx, &vy, &vz);
        vx *= d; vy *= d; vz *= d;
        float px, py, pz;
        px = s1->x + vx; py = s1->y + vy; pz = s1->z + vz;
        float x = s2->x - px;
        float y = s2->y - py;
        float z = s2->z - pz;
        return SDL_sqrtf(x * x + y * y + z * z);
    }

    Player *player_crosshair(Player *player) {
        Player *result = 0;
        float threshold = static_cast<float>(RADIANS(5));
        float best = 0;
        for (int i = 0; i < this->m_model->player_count; i++) {
            Player *other = this->m_model->players + i;
            if (other == player) {
                continue;
            }
            float p = player_crosshair_distance(player, other);
            float d = player_player_distance(player, other);
            if (d < 96 && p / d < threshold) {
                if (best == 0 || d < best) {
                    best = d;
                    result = other;
                }
            }
        }
        return result;
    }

    Chunk *find_chunk(int p, int q) const {
        for (int i = 0; i < this->m_model->chunk_count; i++) {
            Chunk *chunk = this->m_model->chunks + i;
            if (chunk->p == p && chunk->q == q) {
                return chunk;
            }
        }
        return 0;
    }

    int chunk_distance(Chunk *chunk, int p, int q) {
        int dp = ABS(chunk->p - p);
        int dq = ABS(chunk->q - q);
        return SDL_max(dp, dq);
    }

    int chunk_visible(float planes[6][4], int p, int q, int miny, int maxy) {
        float miny_f = static_cast<float>(miny);
        float maxy_f = static_cast<float>(maxy);
        float x = static_cast<float>(p * this->m_gui->chunk_size - 1);
        float z = static_cast<float>(q * this->m_gui->chunk_size - 1);
        float d = static_cast<float>(this->m_gui->chunk_size + 1);
        float points[8][3] = {
            {x + 0.f, miny_f, z + 0.f},
            {x + d, miny_f, z + 0.f},
            {x + 0.f, miny_f, z + d},
            {x + d, miny_f, z + d},
            {x + 0.f, maxy_f, z + 0.f},
            {x + d, maxy_f, z + 0.f},
            {x + 0.f, maxy_f, z + d},
            {x + d, maxy_f, z + d}
        };
        int n = this->m_model->is_ortho ? 4 : 6;
        for (int i = 0; i < n; i++) {
            int in = 0;
            int out = 0;
            for (int j = 0; j < 8; j++) {
                float d =
                    planes[i][0] * points[j][0] +
                    planes[i][1] * points[j][1] +
                    planes[i][2] * points[j][2] +
                    planes[i][3];
                if (d < 0) {
                    out++;
                }
                else {
                    in++;
                }
                if (in && out) {
                    break;
                }
            }
            if (in == 0) {
                return 0;
            }
        }
        return 1;
    } // chunk_visible

    int highest_block(float x, float z) const {
        int result = -1;
        int nx = static_cast<int>(SDL_roundf(x));
        int nz = static_cast<int>(SDL_roundf(z));
        int p = this->chunked(x);
        int q = this->chunked(z);
        Chunk *chunk = this->find_chunk(p, q);
        if (chunk) {
            Map *map = &chunk->map;
            MAP_FOR_EACH(map, ex, ey, ez, ew) {
                // item.h -> is_obstacle
                if (is_obstacle(ew) && ex == nx && ez == nz) {
                    result = SDL_max(result, ey);
                }
            } END_MAP_FOR_EACH;
        }
        return result;
    }

    int _hit_test(Map *map, float max_distance, int previous, float x, float y, float z, float vx, float vy, float vz, int *hx, int *hy, int *hz) {
        static constexpr int m = 32;
        int px = 0;
        int py = 0;
        int pz = 0;
        for (int i = 0; i < max_distance * m; i++) {
            int nx = SDL_lroundf(x);
            int ny = SDL_lroundf(y);
            int nz = SDL_lroundf(z);
            if (nx != px || ny != py || nz != pz) {
                int hw = map_get(map, nx, ny, nz);
                if (hw > 0) {
                    if (previous) {
                        *hx = px; *hy = py; *hz = pz;
                    }
                    else {
                        *hx = nx; *hy = ny; *hz = nz;
                    }
                    return hw;
                }
                px = nx; py = ny; pz = nz;
            }
            x += vx / m; y += vy / m; z += vz / m;
        }
        return 0;
    } // _hit_test

    int hit_test(int previous, float x, float y, float z, float rx, float ry, int *bx, int *by, int *bz) {
        int result = 0;
        float best = 0;
        int p = this->chunked(x);
        int q = this->chunked(z);
        float vx, vy, vz;
        this->get_sight_vector(rx, ry, &vx, &vy, &vz);
        for (int i = 0; i < this->m_model->chunk_count; i++) {
            Chunk *chunk = this->m_model->chunks + i;
            if (this->chunk_distance(chunk, p, q) > 1) {
                continue;
            }
            int hx, hy, hz;
            int hw = this->_hit_test(&chunk->map, 8, previous,
                x, y, z, vx, vy, vz, &hx, &hy, &hz);
            if (hw > 0) {
                float d = SDL_sqrtf(SDL_powf(hx - x, 2) + SDL_powf(hy - y, 2) + SDL_powf(hz - z, 2));
                if (best == 0 || d < best) {
                    best = d;
                    *bx = hx; *by = hy; *bz = hz;
                    result = hw;
                }
            }
        }
        return result;
    } // hit_test

    int hit_test_face(Player *player, int *x, int *y, int *z, int *face) {
        State *s = &player->state;
        int w = this->hit_test(0, s->x, s->y, s->z, s->rx, s->ry, x, y, z);
        // item.h -> is_obstacle
        if (is_obstacle(w)) {
            int hx, hy, hz;
            this->hit_test(1, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
            int dx = hx - *x;
            int dy = hy - *y;
            int dz = hz - *z;
            if (dx == -1 && dy == 0 && dz == 0) {
                *face = 0; return 1;
            }
            if (dx == 1 && dy == 0 && dz == 0) {
                *face = 1; return 1;
            }
            if (dx == 0 && dy == 0 && dz == -1) {
                *face = 2; return 1;
            }
            if (dx == 0 && dy == 0 && dz == 1) {
                *face = 3; return 1;
            }
            if (dx == 0 && dy == 1 && dz == 0) {
                float degrees = SDL_roundf(static_cast<float>(DEGREES(SDL_atan2(s->x - hx, s->z - hz))));
                if (degrees < 0.f) {
                    degrees += 360.f;
                }
                int top = static_cast<int>(((degrees + 45.f) / 90.f)) % 4;
                *face = 4 + top;
                return 1;
            }
        }
        return 0;
    }

    int collide(int height, float *x, float *y, float *z) const {
        int result = 0;
        int p = this->chunked(*x);
        int q = this->chunked(*z);
        Chunk *chunk = this->find_chunk(p, q);
        if (!chunk) {
            return result;
        }
        Map *map = &chunk->map;
        int nx = static_cast<int>(SDL_roundf(*x));
        int ny = static_cast<int>(SDL_roundf(*y));
        int nz = static_cast<int>(SDL_roundf(*z));
        float px = *x - nx;
        float py = *y - ny;
        float pz = *z - nz;
        float pad = 0.25;
        for (int dy = 0; dy < height; dy++) {
            // item.h -> is_obstacle
            if (px < -pad && is_obstacle(map_get(map, nx - 1, ny - dy, nz))) {
                *x = nx - pad;
            }
            if (px > pad && is_obstacle(map_get(map, nx + 1, ny - dy, nz))) {
                *x = nx + pad;
            }
            if (py < -pad && is_obstacle(map_get(map, nx, ny - dy - 1, nz))) {
                *y = ny - pad;
                result = 1;
            }
            if (py > pad && is_obstacle(map_get(map, nx, ny - dy + 1, nz))) {
                *y = ny + pad;
                result = 1;
            }
            if (pz < -pad && is_obstacle(map_get(map, nx, ny - dy, nz - 1))) {
                *z = nz - pad;
            }
            if (pz > pad && is_obstacle(map_get(map, nx, ny - dy, nz + 1))) {
                *z = nz + pad;
            }
        }
        return result;
    }

    int player_intersects_block(int height, float x, float y, float z, int hx, int hy, int hz) const {
        int nx = static_cast<int>(SDL_roundf(x));
        int ny = static_cast<int>(SDL_roundf(y));
        int nz = static_cast<int>(SDL_roundf(z));
        for (int i = 0; i < height; i++) {
            if (nx == hx && ny - i == hy && nz == hz) {
                return 1;
            }
        }
        return 0;
    }

    int _gen_sign_buffer(GLfloat *data, float x, float y, float z, int face, const char *text) const {
        static constexpr int glyph_dx[8] = {0, 0, -1, 1, 1, 0, -1, 0};
        static constexpr int glyph_dz[8] = {1, -1, 0, 0, 0, -1, 0, 1};
        static constexpr int line_dx[8] = {0, 0, 0, 0, 0, 1, 0, -1};
        static constexpr int line_dy[8] = {-1, -1, -1, -1, 0, 0, 0, 0};
        static constexpr int line_dz[8] = {0, 0, 0, 0, 1, 0, -1, 0};
        if (face < 0 || face >= 8) {
            return 0;
        }
        int count = 0;
        float max_width = 64.f;
        float line_height = 1.25f;
        char lines[1024];
        int rows = wrap(text, static_cast<int>(max_width), lines, 1024);
        rows = SDL_min(rows, 5);
        int dx = glyph_dx[face];
        int dz = glyph_dz[face];
        int ldx = line_dx[face];
        int ldy = line_dy[face];
        int ldz = line_dz[face];
        float n = 1.0f / (max_width / 10.f);
        float sx = x - n * static_cast<float>(rows - 1) * (line_height / 2.f) * ldx;
        float sy = y - n * static_cast<float>(rows - 1) * (line_height / 2.f) * ldy;
        float sz = z - n * static_cast<float>(rows - 1) * (line_height / 2.f) * ldz;
        char *key;
        // util.h -> tokenize
        char *line = tokenize(lines, "\n", &key);
        while (line) {
            size_t length = SDL_strlen(line);
            int line_width = string_width(line);
            line_width = static_cast<int>(SDL_min(line_width, max_width));
            float rx = sx - dx * line_width / max_width / 2;
            float ry = sy;
            float rz = sz - dz * line_width / max_width / 2;
            for (int i = 0; i < length; i++) {
                int width = char_width(line[i]);
                line_width -= width;
                if (line_width < 0) {
                    break;
                }
                rx += dx * width / max_width / 2;
                rz += dz * width / max_width / 2;
                if (line[i] != ' ') {
                    make_character_3d(
                        data + count * 30, rx, ry, rz, n / 2, face, line[i]);
                    count++;
                }
                rx += dx * width / max_width / 2;
                rz += dz * width / max_width / 2;
            }
            sx += n * line_height * ldx;
            sy += n * line_height * ldy;
            sz += n * line_height * ldz;
            line = tokenize(NULL, "\n", &key);
            rows--;
            if (rows <= 0) {
                break;
            }
        }
        return count;
    }

    void gen_sign_buffer(Chunk *chunk) const {
        SignList *signs = &chunk->signs;

        // first pass - count characters
        size_t max_faces = 0;
        for (int i = 0; i < signs->size; i++) {
            Sign *e = signs->data + i;
            max_faces += SDL_strlen(e->text);
        }

        // second pass - generate geometry
        GLfloat *data = malloc_faces(5, max_faces);
        size_t faces = 0;
        for (int i = 0; i < signs->size; i++) {
            Sign *e = signs->data + i;
            faces += static_cast<size_t>(this->_gen_sign_buffer(data + static_cast<int>(faces) * 30, 
                static_cast<float>(e->x),
                static_cast<float>(e->y),
                static_cast<float>(e->z), e->face, e->text));
        }

        this->del_buffer(chunk->sign_buffer);
        chunk->sign_buffer = this->gen_faces(5, static_cast<GLsizei>(faces), data);
        chunk->sign_faces = static_cast<int>(faces);
    }

    int has_lights(Chunk *chunk) const {
        if (!this->m_gui->show_lights) {
            return 0;
        }
        for (int dp = -1; dp <= 1; dp++) {
            for (int dq = -1; dq <= 1; dq++) {
                Chunk *other = chunk;
                if (dp || dq) {
                    other = this->find_chunk(chunk->p + dp, chunk->q + dq);
                }
                if (!other) {
                    continue;
                }
                Map *map = &other->lights;
                if (map->size) {
                    return 1;
                }
            }
        }
        return 0;
    }

    void dirty_chunk(Chunk *chunk) const {
        chunk->dirty = 1;
        if (has_lights(chunk)) {
            for (int dp = -1; dp <= 1; dp++) {
                for (int dq = -1; dq <= 1; dq++) {
                    Chunk *other = this->find_chunk(chunk->p + dp, chunk->q + dq);
                    if (other) {
                        other->dirty = 1;
                    }
                }
            }
        }
    }

    void occlusion(char neighbors[27], char lights[27], float shades[27], float ao[6][4], float light[6][4]) const {
        static constexpr int lookup3[6][4][3] = {
            {{0, 1, 3}, {2, 1, 5}, {6, 3, 7}, {8, 5, 7}},
            {{18, 19, 21}, {20, 19, 23}, {24, 21, 25}, {26, 23, 25}},
            {{6, 7, 15}, {8, 7, 17}, {24, 15, 25}, {26, 17, 25}},
            {{0, 1, 9}, {2, 1, 11}, {18, 9, 19}, {20, 11, 19}},
            {{0, 3, 9}, {6, 3, 15}, {18, 9, 21}, {24, 15, 21}},
            {{2, 5, 11}, {8, 5, 17}, {20, 11, 23}, {26, 17, 23}}
        };
        static constexpr int lookup4[6][4][4] = {
            {{0, 1, 3, 4}, {1, 2, 4, 5}, {3, 4, 6, 7}, {4, 5, 7, 8}},
            {{18, 19, 21, 22}, {19, 20, 22, 23}, {21, 22, 24, 25}, {22, 23, 25, 26}},
            {{6, 7, 15, 16}, {7, 8, 16, 17}, {15, 16, 24, 25}, {16, 17, 25, 26}},
            {{0, 1, 9, 10}, {1, 2, 10, 11}, {9, 10, 18, 19}, {10, 11, 19, 20}},
            {{0, 3, 9, 12}, {3, 6, 12, 15}, {9, 12, 18, 21}, {12, 15, 21, 24}},
            {{2, 5, 11, 14}, {5, 8, 14, 17}, {11, 14, 20, 23}, {14, 17, 23, 26}}
        };
        static constexpr float curve[4] = {0.0, 0.25, 0.5, 0.75};
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 4; j++) {
                int corner = neighbors[lookup3[i][j][0]];
                int side1 = neighbors[lookup3[i][j][1]];
                int side2 = neighbors[lookup3[i][j][2]];
                int value = side1 && side2 ? 3 : corner + side1 + side2;
                float shade_sum = 0;
                float light_sum = 0;
                int is_light = lights[13] == 15;
                for (int k = 0; k < 4; k++) {
                    shade_sum += shades[lookup4[i][j][k]];
                    light_sum += lights[lookup4[i][j][k]];
                }
                if (is_light) {
                    light_sum = 15 * 4 * 10;
                }
                float total = curve[value] + shade_sum / 4.0f;
                ao[i][j] = SDL_min(total, 1.0f);
                light[i][j] = light_sum / 15.0f / 4.0f;
            }
        }
    } // occlusion

    void light_fill(char *opaque, char *light, int x, int y, int z, int w, int force) const {
#define XZ_SIZE (this->m_gui->chunk_size * 3 + 2)
#define XZ_LO (this->m_gui->chunk_size)
#define XZ_HI (this->m_gui->chunk_size * 2 + 1)
#define Y_SIZE 258
#define XYZ(x, y, z) ((y) * XZ_SIZE * XZ_SIZE + (x) * XZ_SIZE + (z))
#define XZ(x, z) ((x) * XZ_SIZE + (z))
        if (x + w < XZ_LO || z + w < XZ_LO) {
            return;
        }
        if (x - w > XZ_HI || z - w > XZ_HI) {
            return;
        }
        if (y < 0 || y >= Y_SIZE) {
            return;
        }
        if (light[XYZ(x, y, z)] >= w) {
            return;
        }
        if (!force && opaque[XYZ(x, y, z)]) {
            return;
        }
        light[XYZ(x, y, z)] = w--;
        light_fill(opaque, light, x - 1, y, z, w, 0);
        light_fill(opaque, light, x + 1, y, z, w, 0);
        light_fill(opaque, light, x, y - 1, z, w, 0);
        light_fill(opaque, light, x, y + 1, z, w, 0);
        light_fill(opaque, light, x, y, z - 1, w, 0);
        light_fill(opaque, light, x, y, z + 1, w, 0);
    }

    // Handles terrain generation in a multithreaded environment
    void compute_chunk(WorkerItem *item) const {
        char *opaque = (char *)SDL_calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
        char *light = (char *)SDL_calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
        char *highest = (char *)SDL_calloc(XZ_SIZE * XZ_SIZE, sizeof(char));

        int ox = item->p * this->m_gui->chunk_size - this->m_gui->chunk_size - 1;
        int oy = -1;
        int oz = item->q * this->m_gui->chunk_size - this->m_gui->chunk_size - 1;

        // check for lights
        int has_light = 0;
        if (this->m_gui->show_lights) {
            for (int a = 0; a < 3; a++) {
                for (int b = 0; b < 3; b++) {
                    Map *map = item->light_maps[a][b];
                    if (map && map->size) {
                        has_light = 1;
                    }
                }
            }
        }

        // populate opaque array
        for (int a = 0; a < 3; a++) {
            for (int b = 0; b < 3; b++) {
                Map *block_map = item->block_maps[a][b];
                if (!block_map) {
                    continue;
                }
                MAP_FOR_EACH(block_map, ex, ey, ez, ew) {
                    int x = ex - ox;
                    int y = ey - oy;
                    int z = ez - oz;
                    int w = ew;
                    // TODO: this should be unnecessary
                    if (x < 0 || y < 0 || z < 0) {
                        continue;
                    }
                    if (x >= XZ_SIZE || y >= Y_SIZE || z >= XZ_SIZE) {
                        continue;
                    }
                    // END TODO
                    opaque[XYZ(x, y, z)] = !is_transparent(w);
                    if (opaque[XYZ(x, y, z)]) {
                        highest[XZ(x, z)] = SDL_max(highest[XZ(x, z)], y);
                    }
                } END_MAP_FOR_EACH;
            }
        }

        // flood fill light intensities
        if (has_light) {
            for (int a = 0; a < 3; a++) {
                for (int b = 0; b < 3; b++) {
                    Map *map = item->light_maps[a][b];
                    if (!map) {
                        continue;
                    }
                    MAP_FOR_EACH(map, ex, ey, ez, ew) {
                        int x = ex - ox;
                        int y = ey - oy;
                        int z = ez - oz;
                        light_fill(opaque, light, x, y, z, ew, 1);
                    } END_MAP_FOR_EACH;
                }
            }
        }

        Map *block_map = item->block_maps[1][1];

        // count exposed faces
        int miny = 256;
        int maxy = 0;
        int faces = 0;
        MAP_FOR_EACH(block_map, ex, ey, ez, ew) {
            if (ew <= 0) {
                continue;
            }
            int x = ex - ox;
            int y = ey - oy;
            int z = ez - oz;
            int f1 = !opaque[XYZ(x - 1, y, z)];
            int f2 = !opaque[XYZ(x + 1, y, z)];
            int f3 = !opaque[XYZ(x, y + 1, z)];
            int f4 = !opaque[XYZ(x, y - 1, z)] && (ey > 0);
            int f5 = !opaque[XYZ(x, y, z - 1)];
            int f6 = !opaque[XYZ(x, y, z + 1)];
            int total = f1 + f2 + f3 + f4 + f5 + f6;
            if (total == 0) {
                continue;
            }
            if (is_plant(ew)) {
                total = 4;
            }
            miny = SDL_min(miny, ey);
            maxy = SDL_max(maxy, ey);
            faces += total;
        } END_MAP_FOR_EACH;

        // generate geometry
        // each vertex has 10 components (x, y, z, nx, ny, nz, u, v, ao, light)
        static constexpr int components = 10;
        GLfloat *data = malloc_faces(components, faces);
        int offset = 0;
        MAP_FOR_EACH(block_map, ex, ey, ez, ew) {
            if (ew <= 0) {
                continue;
            }
            int x = ex - ox;
            int y = ey - oy;
            int z = ez - oz;
            int f1 = !opaque[XYZ(x - 1, y, z)];
            int f2 = !opaque[XYZ(x + 1, y, z)];
            int f3 = !opaque[XYZ(x, y + 1, z)];
            int f4 = !opaque[XYZ(x, y - 1, z)] && (ey > 0);
            int f5 = !opaque[XYZ(x, y, z - 1)];
            int f6 = !opaque[XYZ(x, y, z + 1)];
            int total = f1 + f2 + f3 + f4 + f5 + f6;
            if (total == 0) {
                continue;
            }
            char neighbors[27] = {0};
            char lights[27] = {0};
            float shades[27] = {0};
            int index = 0;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dz = -1; dz <= 1; dz++) {
                        neighbors[index] = opaque[XYZ(x + dx, y + dy, z + dz)];
                        lights[index] = light[XYZ(x + dx, y + dy, z + dz)];
                        shades[index] = 0;
                        if (y + dy <= highest[XZ(x + dx, z + dz)]) {
                            for (int oy = 0; oy < 8; oy++) {
                                if (opaque[XYZ(x + dx, y + dy + oy, z + dz)]) {
                                    shades[index] = 1.0f - oy * 0.125f;
                                    break;
                                }
                            }
                        }
                        index++;
                    }
                }
            }
            float ao[6][4];
            float light[6][4];
            occlusion(neighbors, lights, shades, ao, light);
            if (is_plant(ew)) {
                total = 4;
                float min_ao = 1;
                float max_light = 0;
                for (int a = 0; a < 6; a++) {
                    for (int b = 0; b < 4; b++) {
                        min_ao = SDL_min(min_ao, ao[a][b]);
                        max_light = SDL_max(max_light, light[a][b]);
                    }
                }
                float rotation = simplex2(static_cast<float>(ex), static_cast<float>(ez), 4, 0.5f, 2.f) * 360.f;
                make_plant(
                    data + offset, min_ao, max_light,
                    static_cast<float>(ex), static_cast<float>(ey), static_cast<float>(ez),
                    0.5f, ew, rotation);
            }
            else {
                make_cube(
                    data + offset, ao, light,
                    f1, f2, f3, f4, f5, f6,
                    static_cast<float>(ex), static_cast<float>(ey), static_cast<float>(ez), 0.5f, ew);
            }
            offset += total * 60;
        } END_MAP_FOR_EACH;

        SDL_free(opaque);
        SDL_free(light);
        SDL_free(highest);

        item->miny = miny;
        item->maxy = maxy;
        item->faces = faces;
        item->data = data;
    } // compute_chunk

    void generate_chunk(Chunk *chunk, WorkerItem *item) const {
        chunk->miny = item->miny;
        chunk->maxy = item->maxy;
        chunk->faces = item->faces;
        this->del_buffer(chunk->buffer);
        chunk->buffer = this->gen_faces(10, item->faces, item->data);
        this->gen_sign_buffer(chunk);
    }

    void gen_chunk_buffer(Chunk *chunk) const {
        WorkerItem _item;
        WorkerItem *item = &_item;
        item->p = chunk->p;
        item->q = chunk->q;
        for (int dp = -1; dp <= 1; dp++) {
            for (int dq = -1; dq <= 1; dq++) {
                Chunk *other = chunk;
                if (dp || dq) {
                    other = this->find_chunk(chunk->p + dp, chunk->q + dq);
                }
                if (other) {
                    item->block_maps[dp + 1][dq + 1] = &other->map;
                    item->light_maps[dp + 1][dq + 1] = &other->lights;
                }
                else {
                    item->block_maps[dp + 1][dq + 1] = 0;
                    item->light_maps[dp + 1][dq + 1] = 0;
                }
            }
        }
        this->compute_chunk(item);
        this->generate_chunk(chunk, item);
        chunk->dirty = 0;
    }

    static void map_set_func(int x, int y, int z, int w, Map *m) {
        map_set(m, x, y, z, w);
    }

    // Create a chunk that represents a unique portion of the world
    // p, q represents the chunk key
    void load_chunk(WorkerItem *item) {
        if (!this->m_maze) {
			return;
		}
        
        int p = item->p;
        int q = item->q;

        const auto& pq = this->m_maze->get_p_q();
        bool is_part_of_maze = pq.find({ p, q }) != pq.end();
        
        Map *block_map = item->block_maps[1][1];
        Map *light_map = item->light_maps[1][1];
        // world.h
        static world _world;
        auto&& gui_options = this->m_gui;
        _world.create_world(p, q, is_part_of_maze,
            map_set_func, block_map,
            gui_options->chunk_size, gui_options->show_trees, 
            gui_options->show_plants, gui_options->show_clouds);
        db_load_blocks(block_map, p, q);
        db_load_lights(light_map, p, q);
    }

    /**
    * @brief called by ensure_chunk_workers, create_chunk
    * @return
    */
    void init_chunk(Chunk *chunk, int p, int q) {
        chunk->p = p;
        chunk->q = q;
        chunk->faces = 0;
        chunk->sign_faces = 0;
        chunk->buffer = 0;
        chunk->sign_buffer = 0;
        dirty_chunk(chunk);
        SignList *signs = &chunk->signs;
        sign_list_alloc(signs, 16);
        db_load_signs(signs, p, q);
        Map *block_map = &chunk->map;
        Map *light_map = &chunk->lights;
        int dx = p * this->m_gui->chunk_size - 1;
        int dy = 0;
        int dz = q * this->m_gui->chunk_size - 1;
        map_alloc(block_map, dx, dy, dz, 0x7fff);
        map_alloc(light_map, dx, dy, dz, 0xf);
    }

    void create_chunk(Chunk *chunk, int p, int q) {
        init_chunk(chunk, p, q);

        WorkerItem _item;
        WorkerItem *item = &_item;
        item->p = chunk->p;
        item->q = chunk->q;
        item->block_maps[1][1] = &chunk->map;
        item->light_maps[1][1] = &chunk->lights;
        load_chunk(item);
    }

    void delete_chunks() {
        int count = this->m_model->chunk_count;
        State *s1 = &this->m_model->players->state;
        State *s2 = &(this->m_model->players + this->m_model->observe1)->state;
        State *s3 = &(this->m_model->players + this->m_model->observe2)->state;
        State *states[3] = {s1, s2, s3};
        for (int i = 0; i < count; i++) {
            Chunk *chunk = this->m_model->chunks + i;
            int remove_chunk = 1;
            for (int j = 0; j < 3; j++) {
                State *s = states[j];
                int p = chunked(s->x);
                int q = chunked(s->z);
                if (chunk_distance(chunk, p, q) < this->m_model->delete_radius) {
                    remove_chunk = 0;
                    break;
                }
            }
            if (remove_chunk) {
                map_free(&chunk->map);
                map_free(&chunk->lights);
                sign_list_free(&chunk->signs);
                del_buffer(chunk->buffer);
                del_buffer(chunk->sign_buffer);
                Chunk *other = this->m_model->chunks + (--count);
                SDL_memcpy(chunk, other, sizeof(Chunk));
            }
        }
        this->m_model->chunk_count = count;
    }

    /**
     * @brief Deletes all chunks regardless of player state
     *
     */
    void delete_all_chunks() {
        for (int i = 0; i < this->m_model->chunk_count; i++) {
            Chunk *chunk = this->m_model->chunks + i;
            map_free(&chunk->map);
            map_free(&chunk->lights);
            sign_list_free(&chunk->signs);
            del_buffer(chunk->buffer);
            del_buffer(chunk->sign_buffer);
        }
        this->m_model->chunk_count = 0;
    }

    void check_workers() {
        for (auto&& worker : this->m_model->workers) {
            worker->mtx.lock();
            if (worker->state == WORKER_DONE) {
                WorkerItem *item = &worker->item;
                Chunk *chunk = find_chunk(item->p, item->q);
                if (chunk) {
                    if (item->load) {
                        Map *block_map = item->block_maps[1][1];
                        Map *light_map = item->light_maps[1][1];
                        map_free(&chunk->map);
                        map_free(&chunk->lights);
                        map_copy(&chunk->map, block_map);
                        map_copy(&chunk->lights, light_map);
                    }
                    generate_chunk(chunk, item);
                }
                for (int a = 0; a < 3; a++) {
                    for (int b = 0; b < 3; b++) {
                        Map *block_map = item->block_maps[a][b];
                        Map *light_map = item->light_maps[a][b];
                        if (block_map) {
                            map_free(block_map);
                        }
                        if (light_map) {
                            map_free(light_map);
                        }
                    }
                }
                worker->state = WORKER_IDLE;
            }
            worker->mtx.unlock();
        }
    }

    // Used to init the terrain (chunks) around the player -- skip empty maze parts
    void force_chunks(Player *player) {     
        State *s = &player->state;
        int p = chunked(s->x);
        int q = chunked(s->z);

        int r = 1;
        for (int dp = -r; dp <= r; dp++) {
            for (int dq = -r; dq <= r; dq++) {
                int a = p + dp;
                int b = q + dq;
                Chunk *chunk = find_chunk(a, b);
                if (chunk) {
                    if (chunk->dirty) {
                        gen_chunk_buffer(chunk);
                    }
                }
                else if (this->m_model->chunk_count < MAX_CHUNKS) {
                    chunk = this->m_model->chunks + this->m_model->chunk_count++;
                    create_chunk(chunk, a, b);
                    gen_chunk_buffer(chunk);
                }
            }
        }
    }

    /**
     * @brief Calculate  an index based on the chunk coordinates
     *  Check if the chunk is assigned to the current worker thread
     * @param p
     *
     */
    void ensure_chunks_worker(Player *player, Worker *worker) {
        State *s = &player->state;
        float matrix[16];
        set_matrix_3d(matrix, this->m_model->width, this->m_model->height, s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        float planes[6][4];
        frustum_planes(planes, this->m_model->render_radius, matrix);
        int p = chunked(s->x);
        int q = chunked(s->z);
        // int r = this->m_model->create_radius;
        int r {this->m_model->create_radius};
        int start = 0x0fffffff;
        int best_score = start;
        int best_a = 0;
        int best_b = 0;
        for (int dp = -r; dp <= r; dp++) {
            for (int dq = -r; dq <= r; dq++) {
                int a = p + dp;
                int b = q + dq;
                int index = (SDL_abs(a) ^ SDL_abs(b)) % NUM_WORKERS;
                if (index != worker->index) {
                    continue;
                }
                Chunk *chunk = find_chunk(a, b);
                if (chunk && !chunk->dirty) {
                    continue;
                }
                int distance = SDL_max(SDL_abs(dp), SDL_abs(dq));
                int invisible = ~chunk_visible(planes, a, b, 0, 256);
                int priority = 0;
                if (chunk) {
                    priority = chunk->buffer & chunk->dirty;
                }
                // Check for chunk to update based on lowest score
                int score = (invisible << 24) | (priority << 16) | distance;
                if (score < best_score) {
                    best_score = score;
                    best_a = a;
                    best_b = b;
                }
            }
        }
        if (best_score == start) {
            return;
        }
        int a = best_a;
        int b = best_b;
        int load = 0;
        Chunk *chunk = find_chunk(a, b);
        // Check if the chunk is already loaded
        if (!chunk) {
            load = 1;
            if (this->m_model->chunk_count < MAX_CHUNKS) {
                chunk = this->m_model->chunks + this->m_model->chunk_count++;
                init_chunk(chunk, a, b);
            }
            else {
                return;
            }
        }
        WorkerItem *item = &worker->item;
        item->p = chunk->p;
        item->q = chunk->q;
        item->load = load;
        for (int dp = -1; dp <= 1; dp++) {
            for (int dq = -1; dq <= 1; dq++) {
                Chunk *other = chunk;
                if (dp || dq) {
                    other = find_chunk(chunk->p + dp, chunk->q + dq);
                }
                if (other) {
                    // These maps are freed using C-library free function
                    Map *block_map = (Map*) malloc(sizeof(Map));
                    map_copy(block_map, &other->map);
                    Map *light_map = (Map*) malloc(sizeof(Map));
                    map_copy(light_map, &other->lights);
                    item->block_maps[dp + 1][dq + 1] = block_map;
                    item->light_maps[dp + 1][dq + 1] = light_map;
                }
                else {
                    item->block_maps[dp + 1][dq + 1] = 0;
                    item->light_maps[dp + 1][dq + 1] = 0;
                }
            }
        }
        chunk->dirty = 0;
        worker->state = WORKER_BUSY;
        worker->cnd.notify_one();
    } // ensure chunks worker

    void ensure_chunks(Player *player) {
        check_workers();
        force_chunks(player);
        for (auto&& worker : this->m_model->workers) {
            worker->mtx.lock();
            if (worker->state == WORKER_IDLE) {
                ensure_chunks_worker(player, worker.get());
            }
            worker->mtx.unlock();
        }
    }

    void unset_sign(int x, int y, int z) const {
        int p = chunked(static_cast<float>(x));
        int q = chunked(static_cast<float>(z));
        Chunk *chunk = find_chunk(p, q);
        if (chunk) {
            SignList *signs = &chunk->signs;
            if (sign_list_remove_all(signs, x, y, z)) {
                chunk->dirty = 1;
                db_delete_signs(x, y, z);
            }
        }
        else {
            db_delete_signs(x, y, z);
        }
    }

    void unset_sign_face(int x, int y, int z, int face) const {
        int p = chunked(static_cast<float>(x));
        int q = chunked(static_cast<float>(z));
        Chunk *chunk = find_chunk(p, q);
        if (chunk) {
            SignList *signs = &chunk->signs;
            if (sign_list_remove(signs, x, y, z, face)) {
                chunk->dirty = 1;
                db_delete_sign(x, y, z, face);
            }
        }
        else {
            db_delete_sign(x, y, z, face);
        }
    }

    void _set_sign(int p, int q, int x, int y, int z, int face, const char *text, int dirty) const {
        if (SDL_strlen(text) == 0) {
            unset_sign_face(x, y, z, face);
            return;
        }
        Chunk *chunk = find_chunk(p, q);
        if (chunk) {
            SignList *signs = &chunk->signs;
            sign_list_add(signs, x, y, z, face, text);
            if (dirty) {
                chunk->dirty = 1;
            }
        }
        db_insert_sign(p, q, x, y, z, face, text);
    }

    void set_sign(int x, int y, int z, int face, const char *text) const {
        int p = chunked(static_cast<float>(x));
        int q = chunked(static_cast<float>(z));
        _set_sign(p, q, x, y, z, face, text, 1);
    }

    void toggle_light(int x, int y, int z) const {
        int p = chunked(static_cast<float>(x));
        int q = chunked(static_cast<float>(z));
        Chunk *chunk = find_chunk(p, q);
        if (chunk) {
            Map *map = &chunk->lights;
            int w = map_get(map, x, y, z) ? 0 : 15;
            map_set(map, x, y, z, w);
            db_insert_light(p, q, x, y, z, w);
            dirty_chunk(chunk);
        }
    }

    void set_light(int p, int q, int x, int y, int z, int w) const {
        Chunk *chunk = find_chunk(p, q);
        if (chunk) {
            Map *map = &chunk->lights;
            if (map_set(map, x, y, z, w)) {
                dirty_chunk(chunk);
                db_insert_light(p, q, x, y, z, w);
            }
        }
        else {
            db_insert_light(p, q, x, y, z, w);
        }
    }

    void _set_block(int p, int q, int x, int y, int z, int w, int dirty) const {
        Chunk *chunk = find_chunk(p, q);
        if (chunk) {
            Map *map = &chunk->map;
            if (map_set(map, x, y, z, w)) {
                if (dirty) {
                    dirty_chunk(chunk);
                }
                db_insert_block(p, q, x, y, z, w);
            }
        }
        else {
            db_insert_block(p, q, x, y, z, w);
        }
        if (w == 0 && chunked(static_cast<float>(x)) == p && chunked(static_cast<float>(z)) == q) {
            unset_sign(x, y, z);
            set_light(p, q, x, y, z, 0);
        }
    }

    void set_block(int x, int y, int z, int w) const {
        int p = chunked(static_cast<float>(x));
        int q = chunked(static_cast<float>(z));
        _set_block(p, q, x, y, z, w, 1);
        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                if (dx == 0 && dz == 0) {
                    continue;
                }
                if (dx && chunked(static_cast<float>(x + dx)) == p) {
                    continue;
                }
                if (dz && chunked(static_cast<float>(z + dz)) == q) {
                    continue;
                }
                _set_block(p + dx, q + dz, x, y, z, -w, 1);
            }
        }
    }

    // Helper function to modify signs and lights before setting blocks in DB
 //   void _set_blocks_from_vec(const std::vector<std::tuple<int, int, int, int>>& blocks, int dirty) const {
 //       for (auto&& block : blocks) {
 //           int p = get<0>(block);
 //           int q = get<1>(block);
 //           int x = get<2>(block);
 //           int y = get<3>(block);
 //           int z = get<4>(block);
 //           int w = get<5>(block);
 //           Chunk* chunk = find_chunk(p, q);
 //           if (chunk) {
 //               Map* map = &chunk->map;
 //               if (dirty && map_set(map, x, y, z, w)) {
 //                   dirty_chunk(chunk);
 //               }
 //           } else {
 //               // Pass
	//		}
 //           if (w == 0 && chunked(static_cast<float>(x)) == p && chunked(static_cast<float>(z)) == q) {
 //               unset_sign(x, y, z);
 //               set_light(p, q, x, y, z, 0);
 //           }
 //       } // for
 //       db_insert_blocks(blocks);
 //   }

 //   // Tuple(p, q, x, y, z, w)
 //   void set_blocks_from_vec(const std::vector<std::tuple<int, int, int, int, int, int>>& blocks) const {
 //       std::vector<std::tuple<int, int, int, int, int, int>> adjusted_blocks;
 //       // Adjust blocks to account for chunk boundaries
 //       for (auto& block : blocks) {
 //           int x {get<2>(block)};
 //           int z {get<4>(block)};
 //           int p {chunked(static_cast<float>(x))};
 //           int q {chunked(static_cast<float>(z))};
 //           // Check if block is on the edge of the chunk
 //           bool on_the_edge = false;
 //           for (int dx = -1; dx <= 1; dx++) {
 //               for (int dz = -1; dz <= 1; dz++) {
 //                   if ((dx == 0 && dz == 0) || (dx && this->chunked(static_cast<float>(x + dx)) == p) || (dz && this->chunked(static_cast<float>(z + dz)) == q)) {
 //                       continue;
 //                   }
 //                   on_the_edge = true;
 //                   adjusted_blocks.push_back(make_tuple(p + dx, q + dz, x, get<3>(block), z, -get<5>(block)));
 //               }
 //           }
 //           if (!on_the_edge) {
	//			adjusted_blocks.push_back(block);
	//		}
 //       }
 //       _set_blocks_from_vec(cref(adjusted_blocks), 1);
	//}

    void record_block(int x, int y, int z, int w) {
        SDL_memcpy(&this->m_model->block1, &this->m_model->block0, sizeof(Block));
        this->m_model->block0.x = x;
        this->m_model->block0.y = y;
        this->m_model->block0.z = z;
        this->m_model->block0.w = w;
    }

    int get_block(int x, int y, int z) {
        int p = chunked(static_cast<float>(x));
        int q = chunked(static_cast<float>(z));
        Chunk *chunk = find_chunk(p, q);
        if (chunk) {
            Map *map = &chunk->map;
            return map_get(map, x, y, z);
        }
        return 0;
    }

    void builder_block(int x, int y, int z, int w) {
        if (y <= 0 || y >= 256) {
            return;
        }
        if (is_destructable(get_block(x, y, z))) {
            set_block(x, y, z, 0);
        }
        if (w) {
            set_block(x, y, z, w);
        }
    }

    /**
     * @brief Prepares to render by ensuring the chunks are loaded
     *
     */
    int render_chunks(Attrib *attrib, Player *player) {
        int result = 0;
        State *s = &player->state;
        this->ensure_chunks(player);
        int p = this->chunked(s->x);
        int q = this->chunked(s->z);
        float light = this->get_daylight();
        float matrix[16];
        // matrix.cpp -> set_matrix_3d
        set_matrix_3d(
            matrix, this->m_model->width, this->m_model->height,
            s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        float planes[6][4];
        // matrix.cpp -> frustum_planes
        frustum_planes(planes, this->m_model->render_radius, matrix);
        glUseProgram(attrib->program);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        glUniform3f(attrib->camera, s->x, s->y, s->z);
        glUniform1i(attrib->sampler, 0);
        glUniform1i(attrib->extra1, 2);
        glUniform1f(attrib->extra2, light);
        glUniform1f(attrib->extra3, static_cast<GLfloat>(this->m_model->render_radius * this->m_gui->chunk_size));
        glUniform1i(attrib->extra4, static_cast<int>(this->m_model->is_ortho));
        glUniform1f(attrib->timer, this->time_of_day());
        for (int i = 0; i < this->m_model->chunk_count; i++) {
            Chunk *chunk = this->m_model->chunks + i;
            if (this->chunk_distance(chunk, p, q) > this->m_model->render_radius) {
                continue;
            }
            if (!this->chunk_visible(planes, chunk->p, chunk->q, chunk->miny, chunk->maxy)) {
                continue;
            }
            this->draw_chunk(attrib, chunk);
            result += chunk->faces;
        }
        return result;
    }

    void render_signs(Attrib *attrib, Player *player) {
        State *s = &player->state;
        int p = chunked(s->x);
        int q = chunked(s->z);
        float matrix[16];
        set_matrix_3d(
            matrix, this->m_model->width, this->m_model->height,
            s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        float planes[6][4];
        frustum_planes(planes, this->m_model->render_radius, matrix);
        glUseProgram(attrib->program);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        glUniform1i(attrib->sampler, 3);
        glUniform1i(attrib->extra1, 1);
        for (int i = 0; i < this->m_model->chunk_count; i++) {
            Chunk *chunk = this->m_model->chunks + i;
            if (chunk_distance(chunk, p, q) > this->m_model->sign_radius) {
                continue;
            }
            if (!chunk_visible(
                planes, chunk->p, chunk->q, chunk->miny, chunk->maxy))
            {
                continue;
            }
            draw_signs(attrib, chunk);
        }
    }

    void render_sign(Attrib *attrib, Player *player) {
        if (!this->m_model->typing || this->m_model->typing_buffer[0] != CRAFT_KEY_SIGN) {
            return;
        }
        int x, y, z, face;
        if (!hit_test_face(player, &x, &y, &z, &face)) {
            return;
        }
        State *s = &player->state;
        float matrix[16];
        set_matrix_3d(
            matrix, this->m_model->width, this->m_model->height,
            s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        glUseProgram(attrib->program);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        glUniform1i(attrib->sampler, 3);
        glUniform1i(attrib->extra1, 1);
        char text[MAX_SIGN_LENGTH];
        SDL_strlcpy(text, this->m_model->typing_buffer + 1, MAX_SIGN_LENGTH);
        text[MAX_SIGN_LENGTH - 1] = '\0';
        GLfloat *data = malloc_faces(5, SDL_strlen(text));
        int length = _gen_sign_buffer(data, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), face, text);
        GLuint buffer = gen_faces(5, length, data);
        draw_sign(attrib, buffer, length);
        del_buffer(buffer);
    }

    void render_players(Attrib *attrib, Player *player) {
        State *s = &player->state;
        float matrix[16];
        set_matrix_3d(
            matrix, this->m_model->width, this->m_model->height,
            s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        glUseProgram(attrib->program);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        glUniform3f(attrib->camera, s->x, s->y, s->z);
        glUniform1i(attrib->sampler, 0);
        glUniform1f(attrib->timer, time_of_day());
        for (int i = 0; i < this->m_model->player_count; i++) {
            Player *other = this->m_model->players + i;
            if (other != player) {
                draw_player(attrib, other);
            }
        }
    }

    void render_sky(Attrib *attrib, Player *player, GLuint buffer) {
        State *s = &player->state;
        float matrix[16];
        set_matrix_3d(
            matrix, this->m_model->width, this->m_model->height,
            0, 0, 0, s->rx, s->ry, this->m_model->fov, 0, this->m_model->render_radius);
        
        glUseProgram(attrib->program);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        glUniform1i(attrib->sampler, 2);
        glUniform1f(attrib->timer, time_of_day());

        draw_triangles_3d(attrib, buffer, 512 * 3);
    }

    void render_wireframe(Attrib *attrib, Player *player) {
        State *s = &player->state;
        float matrix[16];
        set_matrix_3d(
            matrix, this->m_model->width, this->m_model->height,
            s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        int hx, hy, hz;
        int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        if (is_obstacle(hw)) {
            glUseProgram(attrib->program);
            glLineWidth(1);
            glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
            GLuint wireframe_buffer = gen_wireframe_buffer(static_cast<float>(hx), static_cast<float>(hy), static_cast<float>(hz), 0.53f);
            draw_lines(attrib, wireframe_buffer, 3, 24);
            del_buffer(wireframe_buffer);
        }
    }

    void render_crosshairs(Attrib *attrib) {
        float matrix[16];
        set_matrix_2d(matrix, this->m_model->width, this->m_model->height);
        glUseProgram(attrib->program);
        glLineWidth(static_cast<GLfloat>(4 * this->m_model->scale));
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        GLuint crosshair_buffer = gen_crosshair_buffer();
        draw_lines(attrib, crosshair_buffer, 2, 4);
        del_buffer(crosshair_buffer);
    }

    void render_item(Attrib *attrib) {
        float matrix[16];
        set_matrix_item(matrix, this->m_model->width, this->m_model->height, this->m_model->scale);
        glUseProgram(attrib->program);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        glUniform3f(attrib->camera, 0, 0, 5);
        glUniform1i(attrib->sampler, 0);
        glUniform1f(attrib->timer, time_of_day());
        int w = items[this->m_model->item_index];
        if (is_plant(w)) {
            GLuint buffer = gen_plant_buffer(0, 0, 0, 0.5, w);
            draw_plant(attrib, buffer);
            del_buffer(buffer);
        }
        else {
            GLuint buffer = gen_cube_buffer(0, 0, 0, 0.5, w);
            draw_cube(attrib, buffer);
            del_buffer(buffer);
        }
    }

    void render_text(Attrib *attrib, int justify, float x, float y, float n, char *text) {
        float matrix[16];
        set_matrix_2d(matrix, this->m_model->width, this->m_model->height);
        glUseProgram(attrib->program);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        glUniform1i(attrib->sampler, 1);
        glUniform1i(attrib->extra1, 0);
        GLsizei length = static_cast<GLsizei>(SDL_strlen(text));
        x -= n * justify * (length - 1) / 2;
        GLuint buffer = gen_text_buffer(x, y, n, text);
        draw_text(attrib, buffer, length);
        del_buffer(buffer);
    }

    void add_message(const char *text) {
        SDL_snprintf(this->m_model->messages[this->m_model->message_index], MAX_TEXT_LENGTH, "%s", text);
        this->m_model->message_index = (this->m_model->message_index + 1) % MAX_MESSAGES;
    }

    void copy() {
        SDL_memcpy(&this->m_model->copy0, &this->m_model->block0, sizeof(Block));
        SDL_memcpy(&this->m_model->copy1, &this->m_model->block1, sizeof(Block));
    }

    void paste() {
        Block *c1 = &this->m_model->copy1;
        Block *c2 = &this->m_model->copy0;
        Block *p1 = &this->m_model->block1;
        Block *p2 = &this->m_model->block0;
        int scx = SIGN(c2->x - c1->x);
        int scz = SIGN(c2->z - c1->z);
        int spx = SIGN(p2->x - p1->x);
        int spz = SIGN(p2->z - p1->z);
        int oy = p1->y - c1->y;
        int dx = ABS(c2->x - c1->x);
        int dz = ABS(c2->z - c1->z);
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x <= dx; x++) {
                for (int z = 0; z <= dz; z++) {
                    int w = get_block(c1->x + x * scx, y, c1->z + z * scz);
                    builder_block(p1->x + x * spx, y + oy, p1->z + z * spz, w);
                }
            }
        }
    }

    void array(Block *b1, Block *b2, int xc, int yc, int zc) {
        if (b1->w != b2->w) {
            return;
        }
        int w = b1->w;
        int dx = b2->x - b1->x;
        int dy = b2->y - b1->y;
        int dz = b2->z - b1->z;
        xc = dx ? xc : 1;
        yc = dy ? yc : 1;
        zc = dz ? zc : 1;
        for (int i = 0; i < xc; i++) {
            int x = b1->x + dx * i;
            for (int j = 0; j < yc; j++) {
                int y = b1->y + dy * j;
                for (int k = 0; k < zc; k++) {
                    int z = b1->z + dz * k;
                    builder_block(x, y, z, w);
                }
            }
        }
    }

    void cube(Block *b1, Block *b2, int fill) {
        if (b1->w != b2->w) {
            return;
        }
        int w = b1->w;
        int x1 = SDL_min(b1->x, b2->x);
        int y1 = SDL_min(b1->y, b2->y);
        int z1 = SDL_min(b1->z, b2->z);
        int x2 = SDL_max(b1->x, b2->x);
        int y2 = SDL_max(b1->y, b2->y);
        int z2 = SDL_max(b1->z, b2->z);
        int a = (x1 == x2) + (y1 == y2) + (z1 == z2);
        for (int x = x1; x <= x2; x++) {
            for (int y = y1; y <= y2; y++) {
                for (int z = z1; z <= z2; z++) {
                    if (!fill) {
                        int n = 0;
                        n += x == x1 || x == x2;
                        n += y == y1 || y == y2;
                        n += z == z1 || z == z2;
                        if (n <= a) {
                            continue;
                        }
                    }
                    builder_block(x, y, z, w);
                }
            }
        }
    }

    void sphere(Block *center, int radius, int fill, int fx, int fy, int fz) {
        static const float offsets[8][3] = {
            {-0.5, -0.5, -0.5},
            {-0.5, -0.5, 0.5},
            {-0.5, 0.5, -0.5},
            {-0.5, 0.5, 0.5},
            {0.5, -0.5, -0.5},
            {0.5, -0.5, 0.5},
            {0.5, 0.5, -0.5},
            {0.5, 0.5, 0.5}
        };
        int cx = center->x;
        int cy = center->y;
        int cz = center->z;
        int w = center->w;
        for (int x = cx - radius; x <= cx + radius; x++) {
            if (fx && x != cx) {
                continue;
            }
            for (int y = cy - radius; y <= cy + radius; y++) {
                if (fy && y != cy) {
                    continue;
                }
                for (int z = cz - radius; z <= cz + radius; z++) {
                    if (fz && z != cz) {
                        continue;
                    }
                    int inside = 0;
                    int outside = fill;
                    for (int i = 0; i < 8; i++) {
                        float dx = x + offsets[i][0] - cx;
                        float dy = y + offsets[i][1] - cy;
                        float dz = z + offsets[i][2] - cz;
                        float d = SDL_sqrtf(dx * dx + dy * dy + dz * dz);
                        if (d < radius) {
                            inside = 1;
                        }
                        else {
                            outside = 1;
                        }
                    }
                    if (inside && outside) {
                        builder_block(x, y, z, w);
                    }
                }
            }
        }
    }

    void cylinder(Block *b1, Block *b2, int radius, int fill) {
        if (b1->w != b2->w) {
            return;
        }
        int w = b1->w;
        int x1 = SDL_min(b1->x, b2->x);
        int y1 = SDL_min(b1->y, b2->y);
        int z1 = SDL_min(b1->z, b2->z);
        int x2 = SDL_max(b1->x, b2->x);
        int y2 = SDL_max(b1->y, b2->y);
        int z2 = SDL_max(b1->z, b2->z);
        int fx = x1 != x2;
        int fy = y1 != y2;
        int fz = z1 != z2;
        if (fx + fy + fz != 1) {
            return;
        }
        Block block = {x1, y1, z1, w};
        if (fx) {
            for (int x = x1; x <= x2; x++) {
                block.x = x;
                sphere(&block, radius, fill, 1, 0, 0);
            }
        }
        if (fy) {
            for (int y = y1; y <= y2; y++) {
                block.y = y;
                sphere(&block, radius, fill, 0, 1, 0);
            }
        }
        if (fz) {
            for (int z = z1; z <= z2; z++) {
                block.z = z;
                sphere(&block, radius, fill, 0, 0, 1);
            }
        }
    }

    void tree(Block *block) {
        int bx = block->x;
        int by = block->y;
        int bz = block->z;
        for (int y = by + 3; y < by + 8; y++) {
            for (int dx = -3; dx <= 3; dx++) {
                for (int dz = -3; dz <= 3; dz++) {
                    int dy = y - (by + 4);
                    int d = (dx * dx) + (dy * dy) + (dz * dz);
                    if (d < 11) {
                        builder_block(bx + dx, y, bz + dz, 15);
                    }
                }
            }
        }
        for (int y = by; y < by + 7; y++) {
            builder_block(bx, y, bz, 5);
        }
    }

    void maze(int w, int l, int h) {
        mt19937 rng{ static_cast<unsigned long>(this->m_gui->seed) };
        auto get_int = [&rng](int low, int high)->int {
            uniform_int_distribution<int> dist{ low, high };
            return dist(rng);
        };
        auto get_maze_type = [](auto str)->mazes::maze_types {
			if (SDL_strcmp(str.c_str(), "binary_tree") == 0) {
				return maze_types::BINARY_TREE;
			} else if (SDL_strcmp(str.c_str(), "sidewinder") == 0) {
				return maze_types::SIDEWINDER;
			} else {
				return maze_types::INVALID_ALGO;
			}
		};
        maze_thread_safe mz {get_maze_type(this->m_gui->maze_algo), cref(get_int), cref(rng), 
            static_cast<unsigned int>(w), static_cast<unsigned int>(l), static_cast<unsigned int>(h)};
        auto&& vertices = mz.get_render_vertices();
        auto set_maze_in_craft = [this](const vector<tuple<int, int, int, int>>& vertices) {
            for (auto&& block : vertices) {
                // Set the block in the DB
                this->set_block(get<0>(block), get<1>(block), get<2>(block), get<3>(block));
                // Record the block in craft
                this->record_block(get<0>(block), get<1>(block), get<2>(block), get<3>(block));
            }
        };
        set_maze_in_craft(cref(vertices));
    }

    void parse_command(const char *buffer) {
        int radius, count, xc, yc, zc;
        if (SDL_sscanf(buffer, "/view %d", &radius) == 1) {
            if (radius >= 1 && radius <= 24) {
                this->m_model->create_radius = radius;
                this->m_model->render_radius = radius;
                this->m_model->delete_radius = radius + 4;
            }
            else {
                add_message("Viewing distance must be between 1 and 24.");
            }
        }
        else if (SDL_strcmp(buffer, "/copy") == 0) {
            copy();
        }
        else if (SDL_strcmp(buffer, "/paste") == 0) {
            paste();
        }
        else if (SDL_strcmp(buffer, "/tree") == 0) {
            tree(&this->m_model->block0);
        }
        else if (SDL_sscanf(buffer, "/move %d %d %d", &xc, &yc, &zc) == 3) {
            auto&& ps = this->m_model->players->state;
            ps.x = xc;
            ps.y = yc;
            ps.z = zc;
#if defined(MAZE_DEBUG)
            SDL_Log("/move (%d, %d, %d)", xc, yc, zc);
#endif
        }
        else if (SDL_sscanf(buffer, "/array %d %d %d", &xc, &yc, &zc) == 3) {
            array(&this->m_model->block1, &this->m_model->block0, xc, yc, zc);
        }
        else if (SDL_sscanf(buffer, "/array %d", &count) == 1) {
            array(&this->m_model->block1, &this->m_model->block0, count, count, count);
        }
        else if (SDL_strcmp(buffer, "/fcube") == 0) {
            cube(&this->m_model->block0, &this->m_model->block1, 1);
        }
        else if (SDL_strcmp(buffer, "/cube") == 0) {
            cube(&this->m_model->block0, &this->m_model->block1, 0);
        }
        else if (SDL_sscanf(buffer, "/fsphere %d", &radius) == 1) {
            sphere(&this->m_model->block0, radius, 1, 0, 0, 0);
        }
        else if (SDL_sscanf(buffer, "/sphere %d", &radius) == 1) {
            sphere(&this->m_model->block0, radius, 0, 0, 0, 0);
        }
        else if (SDL_sscanf(buffer, "/fcirclex %d", &radius) == 1) {
            sphere(&this->m_model->block0, radius, 1, 1, 0, 0);
        }
        else if (SDL_sscanf(buffer, "/circlex %d", &radius) == 1) {
            sphere(&this->m_model->block0, radius, 0, 1, 0, 0);
        }
        else if (SDL_sscanf(buffer, "/fcircley %d", &radius) == 1) {
            sphere(&this->m_model->block0, radius, 1, 0, 1, 0);
        }
        else if (SDL_sscanf(buffer, "/circley %d", &radius) == 1) {
            sphere(&this->m_model->block0, radius, 0, 0, 1, 0);
        }
        else if (SDL_sscanf(buffer, "/fcirclez %d", &radius) == 1) {
            sphere(&this->m_model->block0, radius, 1, 0, 0, 1);
        }
        else if (SDL_sscanf(buffer, "/circlez %d", &radius) == 1) {
            sphere(&this->m_model->block0, radius, 0, 0, 0, 1);
        }
        else if (SDL_sscanf(buffer, "/fcylinder %d", &radius) == 1) {
            cylinder(&this->m_model->block0, &this->m_model->block1, radius, 1);
        }
        else if (SDL_sscanf(buffer, "/cylinder %d", &radius) == 1) {
            cylinder(&this->m_model->block0, &this->m_model->block1, radius, 0);
        }
        else if (SDL_sscanf(buffer, "/maze %d %d %d", &xc, &yc, &zc) == 3) {
            maze(xc, yc, zc);
        }
    } // prase command

    void on_light() {
        State *s = &this->m_model->players->state;
        int hx, hy, hz;
        int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        if (hy > 0 && hy < 256 && is_destructable(hw)) {
            toggle_light(hx, hy, hz);
        }
    }

    void on_left_click() {
        State *s = &this->m_model->players->state;
        int hx, hy, hz;
        int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        if (hy > 0 && hy < 256 && is_destructable(hw)) {
            set_block(hx, hy, hz, 0);
            record_block(hx, hy, hz, 0);
#if defined(MAZE_DEBUG)
            SDL_Log("on_left_click(%d, %d, %d, %d, block_type: %d): ", hx, hy, hz, hw, items[this->m_model->item_index]);
#endif
            if (is_plant(get_block(hx, hy + 1, hz))) {
                set_block(hx, hy + 1, hz, 0);
            }
        }
    }

    void on_right_click() {
        State *s = &this->m_model->players->state;
        int hx, hy, hz;
        int hw = hit_test(1, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        if (hy > 0 && hy < 256 && is_obstacle(hw)) {
            if (!player_intersects_block(2, s->x, s->y, s->z, hx, hy, hz)) {
                set_block(hx, hy, hz, items[this->m_model->item_index]);
                record_block(hx, hy, hz, items[this->m_model->item_index]);
#if defined(MAZE_DEBUG)
                SDL_Log("on_right_click(%d, %d, %d, %d, block_type: %d): ", hx, hy, hz, hw, items[this->m_model->item_index]);
#endif
            }
        }
    }

    void on_middle_click() {
        State *s = &this->m_model->players->state;
        int hx, hy, hz;
        int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        for (int i = 0; i < item_count; i++) {
            if (items[i] == hw) {
                this->m_model->item_index = i;
                break;
            }
        }
    }

    /**
    * reference: https://github.com/rswinkle/Craft/blob/sdl/src/main.c
    * @brief Handle SDL events
    * @param dt
    * @param running is a reference to game loop invariant
    * @return bool return true when events are handled successfully
    */
    bool handle_events(double dt, bool& running) {
        static float dy = 0;
        State* s = &this->m_model->players->state;
        int sz = 0;
        int sx = 0;
        float mouse_mv = 0.0025f;
        float dir_mv = 0.025f;
        int sc = -1, code = -1;

        SDL_Keymod mod_state = SDL_GetModState();

        int control = mod_state;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);
            switch (e.type) {
            case SDL_EVENT_QUIT: {
                running = false;
                break;
            }
            case SDL_EVENT_KEY_UP: {
                sc = e.key.scancode;
                switch (sc) {
                }
                break;
            }
            case SDL_EVENT_KEY_DOWN: {

                sc = e.key.scancode;
                switch (sc) {
                case SDL_SCANCODE_ESCAPE: {
                    SDL_SetWindowRelativeMouseMode(this->m_model->window, SDL_FALSE);
                    this->m_gui->capture_mouse = false;
                    this->m_gui->fullscreen = false;
                    this->m_model->typing = 0;
                    break;
                }
                case SDL_SCANCODE_RETURN: {
                    if (this->m_model->typing) {
                        if (mod_state) {
                            if (this->m_model->text_len < MAX_TEXT_LENGTH - 1) {
                                this->m_model->typing_buffer[this->m_model->text_len] = '\n';
                                this->m_model->typing_buffer[this->m_model->text_len + 1] = '\0';
                            }
                        } else {
                            this->m_model->typing = 0;
                            if (this->m_model->typing_buffer[0] == CRAFT_KEY_SIGN) {
                                Player* player = this->m_model->players;
                                int x, y, z, face;
                                if (hit_test_face(player, &x, &y, &z, &face)) {
                                    set_sign(x, y, z, face, this->m_model->typing_buffer + 1);
                                }
                            } else if (this->m_model->typing_buffer[0] == '/') {

                                this->parse_command(this->m_model->typing_buffer);
                            }
                        }
                    } else {
                        if (control) {
                            this->on_right_click();
                        } else {
                            this->on_left_click();
                        }
                    }
                    break;
                }
                case SDL_SCANCODE_V: {
                    if (control) {
                        auto clip_buffer = const_cast<char*>(SDL_GetClipboardText());
                        if (this->m_model->typing) {
                            this->m_model->suppress_char = 1;
                            SDL_strlcat(this->m_model->typing_buffer, clip_buffer,
                                MAX_TEXT_LENGTH - this->m_model->text_len - 1);
                        } else {
                            parse_command(clip_buffer);
                        }
                        SDL_free(clip_buffer);
                    }
                    break;
                }
                case SDL_SCANCODE_0:
                case SDL_SCANCODE_1:
                case SDL_SCANCODE_2:
                case SDL_SCANCODE_3:
                case SDL_SCANCODE_4:
                case SDL_SCANCODE_5:
                case SDL_SCANCODE_6:
                case SDL_SCANCODE_7:
                case SDL_SCANCODE_8:
                case SDL_SCANCODE_9: {
                    if (this->m_gui->capture_mouse && !this->m_model->typing)
                        this->m_model->item_index = (sc - SDL_SCANCODE_1);
                    break;
                }
                case KEY_FLY: {
                    if (!this->m_model->typing && this->m_gui->capture_mouse)
                        this->m_model->flying = ~this->m_model->flying;
                    break;
                }
                case KEY_ITEM_NEXT: {
                    if (!this->m_model->typing && this->m_gui->capture_mouse)
                        this->m_model->item_index = (this->m_model->item_index + 1) % item_count;
                    break;
                }
                case KEY_ITEM_PREV: {
                    if (!this->m_model->typing && this->m_gui->capture_mouse) {
                        this->m_model->item_index--;
                        if (this->m_model->item_index < 0)
                            this->m_model->item_index = item_count - 1;
                    }
                    break;
                }
                case KEY_OBSERVE: {
                    if (!this->m_model->typing && this->m_gui->capture_mouse)
                        this->m_model->observe1 = (this->m_model->observe1 + 1) % this->m_model->player_count;
                    break;
                }
                case KEY_OBSERVE_INSET: {
                    if (!this->m_model->typing && this->m_gui->capture_mouse)
                        this->m_model->observe2 = (this->m_model->observe2 + 1) % this->m_model->player_count;
                    break;
                }
                case KEY_CHAT: {
                    if (!this->m_model->typing && this->m_gui->capture_mouse) {
                        this->m_model->typing = 1;
                        this->m_model->typing_buffer[0] = '\0';
                        this->m_model->text_len = 0;
                        SDL_StartTextInput(this->m_model->window);
                    }
                    break;
                }
                case KEY_COMMAND: {
                    if (!this->m_model->typing && this->m_gui->capture_mouse) {
                        this->m_model->typing = 1;
                        this->m_model->typing_buffer[0] = '\0';
                        SDL_StartTextInput(this->m_model->window);
                    }
                    break;
                }
                case KEY_SIGN: {
                    if (!this->m_model->typing && this->m_gui->capture_mouse) {
                        this->m_model->typing = 1;
                        this->m_model->typing_buffer[0] = '\0';
                        SDL_StartTextInput(this->m_model->window);
                    }
                    break;

                }
                } // switch
                break;
            } // case SDL_EVENT_KEY_DOWN
            case SDL_EVENT_TEXT_INPUT: {
                if (this->m_gui->capture_mouse && this->m_model->typing && this->m_model->text_len < MAX_TEXT_LENGTH - 1) {
                    SDL_strlcat(this->m_model->typing_buffer, e.text.text, this->m_model->text_len);
                    this->m_model->text_len += SDL_strlen(e.text.text);
                }
                break;
            }
            case SDL_EVENT_MOUSE_MOTION: {
                if (this->m_gui->capture_mouse && SDL_GetWindowRelativeMouseMode(this->m_model->window)) {
                    s->rx += e.motion.xrel * mouse_mv;
                    if (INVERT_MOUSE) {
                        s->ry += e.motion.yrel * mouse_mv;
                    } else {
                        s->ry -= e.motion.yrel * mouse_mv;
                    }
                    if (s->rx < 0) {
                        s->rx += RADIANS(360);
                    }
                    if (s->rx >= RADIANS(360)) {
                        s->rx -= RADIANS(360);
                    }
                    s->ry = SDL_max(s->ry, -RADIANS(90));
                    s->ry = SDL_min(s->ry, RADIANS(90));
                }
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                if (this->m_gui->capture_mouse && SDL_GetWindowRelativeMouseMode(this->m_model->window) && e.button.button == SDL_BUTTON_LEFT) {
                    if (control) {
                        on_right_click();
                    } else {
                        on_left_click();
                    }
                } else if (this->m_gui->capture_mouse && SDL_GetWindowRelativeMouseMode(this->m_model->window) && e.button.button == SDL_BUTTON_RIGHT) {
                    if (control) {
                        on_light();
                    } else {
                        on_right_click();
                    }
                } else if (e.button.button == SDL_BUTTON_MIDDLE) {
                    if (this->m_gui->capture_mouse && SDL_GetWindowRelativeMouseMode(this->m_model->window)) {
                        on_middle_click();
                    }
                }

                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                if (this->m_gui->capture_mouse && SDL_GetWindowRelativeMouseMode(this->m_model->window)) {
                    // TODO might have to change this to force 1 step
                    if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL) {
                        this->m_model->item_index += e.wheel.y;
                    } else {
                        this->m_model->item_index -= e.wheel.y;
                    }
                    if (this->m_model->item_index < 0)
                        this->m_model->item_index = item_count - 1;
                    else
                        this->m_model->item_index %= item_count;
                }
                break;
            }
            case SDL_EVENT_WINDOW_RESIZED: {
                this->m_model->scale = get_scale_factor();
                SDL_GetWindowSizeInPixels(this->m_model->window, &this->m_model->width, &this->m_model->height);
                break;
            }
            case SDL_EVENT_WINDOW_SHOWN: {
                this->m_model->scale = get_scale_factor();
                SDL_GetWindowSizeInPixels(this->m_model->window, &this->m_model->width, &this->m_model->height);
                break;
            }
            } // switch
        } // SDL_Event

        // Close the app, events handled successfully
        if (!running) {
            return true;
        }

        const Uint8* state = SDL_GetKeyboardState(nullptr);

        if (!this->m_model->typing && this->m_gui->capture_mouse) {
            this->m_model->is_ortho = state[KEY_ORTHO] ? 64 : 0;
            this->m_model->fov = state[KEY_ZOOM] ? 15 : 65;
            if (state[KEY_FORWARD]) sz--;
            if (state[KEY_BACKWARD]) sz++;
            if (state[KEY_LEFT]) sx--;
            if (state[KEY_RIGHT]) sx++;
            if (state[SDL_SCANCODE_LEFT]) s->rx -= dir_mv;
            if (state[SDL_SCANCODE_RIGHT]) s->rx += dir_mv;
            if (state[SDL_SCANCODE_UP]) s->ry += dir_mv;
            if (state[SDL_SCANCODE_DOWN]) s->ry -= dir_mv;
        }

        float vx, vy, vz;
        get_motion_vector(this->m_model->flying, sz, sx, s->rx, s->ry, &vx, &vy, &vz);
        if (!this->m_model->typing) {
            if (state[KEY_JUMP] && this->m_gui->capture_mouse) {
                if (this->m_model->flying) {
                    vy = 1;
                } else if (dy == 0) {
                    dy = 8;
                }
            }
        }
        float speed = this->m_model->flying ? 20 : 5;
        int estimate = SDL_roundf(SDL_sqrtf(
            SDL_powf(vx * speed, 2) +
            SDL_powf(vy * speed + SDL_abs(dy) * 2, 2) +
            SDL_powf(vz * speed, 2)) * dt * 8);
        int step = SDL_max(8, estimate);
        float ut = dt / step;
        vx = vx * ut * speed;
        vy = vy * ut * speed;
        vz = vz * ut * speed;
        for (int i = 0; i < step; i++) {
            if (this->m_model->flying) {
                dy = 0;
            } else {
                dy -= ut * 25;
                dy = SDL_max(dy, -250);
            }
            s->x += vx;
            s->y += vy + dy * ut;
            s->z += vz;
            if (collide(2, &s->x, &s->y, &s->z)) {
                dy = 0;
            }
        }
        if (s->y < 0) {
            s->y = highest_block(s->x, s->z) + 2;
        }

        return true;
    } // handle_events

    /**
     * @brief Check what fullscreen modes are avaialble
     */
    void check_fullscreen_modes() {
        SDL_DisplayID display = SDL_GetPrimaryDisplay();
        int num_modes = 0;
        const SDL_DisplayMode * const *modes = SDL_GetFullscreenDisplayModes(display, &num_modes);
        if (modes) {
            for (int i = 0; i < num_modes; ++i) {
                const SDL_DisplayMode *mode = modes[i];
                SDL_Log("Display %" SDL_PRIu32 " mode %d: %dx%d@%gx %gHz\n",
                        display, i, mode->w, mode->h, mode->pixel_density, mode->refresh_rate);
            }
        }
    }

    /**
     * @brief Create SDL/GL window and context, check display modes
     */
    void create_window_and_context() {
        this->m_model->start_ticks = static_cast<int>(SDL_GetTicks());
        Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
        int window_width = INIT_WINDOW_WIDTH;
        int window_height = INIT_WINDOW_HEIGHT;

#if defined(MAZE_DEBUG)
        SDL_Log("Settings SDL_GL_CONTEXT_DEBUG_FLAG\n");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
#endif

#if defined(__EMSCRIPTEN__)
#if defined(MAZE_DEBUG)
        SDL_Log("Setting SDL_GL_CONTEXT_PROFILE_ES\n");
#endif
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
#if defined(MAZE_DEBUG)
        SDL_Log("Setting SDL_GL_CONTEXT_PROFILE_CORE\n");
#endif
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        
        this->m_model->window = SDL_CreateWindow(m_window_name.data(), window_width, window_height, window_flags);
        if (this->m_model->window == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateWindow failed (%s)\n", SDL_GetError());
        }
        this->m_model->context = SDL_GL_CreateContext(this->m_model->window);

        if (this->m_model->context == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_GL_CreateContext failed (%s)\n", SDL_GetError());
        }

        SDL_GL_MakeCurrent(this->m_model->window, this->m_model->context);

        SDL_GL_SetSwapInterval(this->m_gui->vsync);

        auto icon_path{ "textures/maze_in_green_32x32.bmp" };
        SDL_Surface *icon_surface = SDL_LoadBMP_IO(SDL_IOFromFile(icon_path, "rb"), SDL_TRUE);
        if (icon_surface) {
            SDL_SetWindowIcon(this->m_model->window, icon_surface);
            SDL_DestroySurface(icon_surface);
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ERROR: Couldn't load icon at %s\n", icon_path);
        }
    } // create_window_and_context

    void reset_model() {
        SDL_memset(this->m_model->chunks, 0, sizeof(Chunk) * MAX_CHUNKS);
        this->m_model->chunk_count = 0;
        SDL_memset(this->m_model->players, 0, sizeof(Player) * MAX_PLAYERS);
        this->m_model->player_count = 0;
        this->m_model->observe1 = 0;
        this->m_model->observe2 = 0;
        this->m_model->flying = 0;
        this->m_model->item_index = 0;
        SDL_memset(this->m_model->typing_buffer, 0, sizeof(char) * MAX_TEXT_LENGTH);
        this->m_model->typing = 0;
        SDL_memset(this->m_model->messages, 0, sizeof(char) * MAX_MESSAGES * MAX_TEXT_LENGTH);
        this->m_model->message_index = 0;
        this->m_model->day_length = DAY_LENGTH;
        this->m_model->start_time = (this->m_model->day_length / 3)*1000;
        // maybe set start_ticks here?
        this->m_model->time_changed = 1;
    }

}; // craft_impl

craft::craft(const std::string& window_name, const std::string& version, const std::string& help)
    : m_pimpl{std::make_unique<craft_impl>(window_name, version, help)} {
}

craft::~craft() = default;


/**
 * Run the craft-engine in a loop with SDL window open, compute the maze first
*/
bool craft::run(unsigned long seed, const std::list<std::string>& algos, const std::function<mazes::maze_types(const std::string& algo)> get_maze_algo_from_str) const noexcept {
    // Init RNG engine
    std::mt19937 rng_machine{ seed };
    this->m_pimpl->m_gui->seed = seed;
    auto get_int = [&rng_machine](int min, int max) {
		std::uniform_int_distribution<int> dist{ min, max };
		return dist(rng_machine);
	};

    // SDL INITIALIZATION //
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_Init failed (%s)\n", SDL_GetError());
        return false;
    }

    m_pimpl->create_window_and_context();
    if (!m_pimpl->m_model->window) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_Window failed (%s)\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    SDL_ShowWindow(m_pimpl->m_model->window);
    SDL_SetWindowRelativeMouseMode(this->m_pimpl->m_model->window, SDL_FALSE);

#if !defined(__EMSCRIPTEN__)
    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "OpenGL loader failed (%s)\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
#endif

//#if defined(MAZE_DEBUG)
//    this->m_pimpl->check_fullscreen_modes();
//#endif

#if defined(MAZE_DEBUG)
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
	
	SDL_Log("-------------------------------------------------------------\n");
    SDL_Log("GL Vendor    : %s\n", vendor);
    SDL_Log("GL Renderer  : %s\n", renderer);
    SDL_Log("GL Version   : %s\n", version);
    SDL_Log("GL Version   : %d.%d\n", major, minor);
    SDL_Log("GLSL Version : %s\n", glslVersion);
    SDL_Log("-------------------------------------------------------------\n");
    bool dump_exts = false;
    if (dump_exts) {
        GLint nExtensions;
        glGetIntegerv(GL_NUM_EXTENSIONS, &nExtensions);
        for (int i = 0; i < nExtensions; i++) {
            SDL_Log("%s\n", glGetStringi(GL_EXTENSIONS, i));
        }
    }
#endif

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 1);

    // LOAD TEXTURES
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    load_png_texture("textures/texture.png");

    GLuint font;
    glGenTextures(1, &font);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, font);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    load_png_texture("textures/font.png");

    GLuint sky;
    glGenTextures(1, &sky);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, sky);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    load_png_texture("textures/sky.png");

    GLuint sign;
    glGenTextures(1, &sign);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, sign);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    load_png_texture("textures/sign.png");

    // LOAD SHADERS 
    craft_impl::Attrib block_attrib = {0};
    craft_impl::Attrib line_attrib = {0};
    craft_impl::Attrib text_attrib = {0};
    craft_impl::Attrib sky_attrib = {0};

    GLuint program;

#if defined(__EMSCRIPTEN__)
    program = load_program("shaders/es/block_vertex.es.glsl", "shaders/es/block_fragment.es.glsl");
#else
    program = load_program("shaders/block_vertex.glsl", "shaders/block_fragment.glsl");
#endif
    block_attrib.program = program;
    block_attrib.position = 0;
    block_attrib.normal = 1;
    block_attrib.uv = 2;
    block_attrib.matrix = glGetUniformLocation(program, "matrix");
    block_attrib.sampler = glGetUniformLocation(program, "sampler");
    block_attrib.extra1 = glGetUniformLocation(program, "sky_sampler");
    block_attrib.extra2 = glGetUniformLocation(program, "daylight");
    block_attrib.extra3 = glGetUniformLocation(program, "fog_distance");
    block_attrib.extra4 = glGetUniformLocation(program, "is_ortho");
    block_attrib.camera = glGetUniformLocation(program, "camera");
    block_attrib.timer = glGetUniformLocation(program, "timer");

#if defined(__EMSCRIPTEN__)
    program = load_program("shaders/es/line_vertex.es.glsl", "shaders/es/line_fragment.es.glsl");
#else
    program = load_program("shaders/line_vertex.glsl", "shaders/line_fragment.glsl");
#endif
    line_attrib.program = program;
    line_attrib.position = 0;
    line_attrib.matrix = glGetUniformLocation(program, "matrix");

#if defined(__EMSCRIPTEN__)
    program = load_program("shaders/es/text_vertex.es.glsl", "shaders/es/text_fragment.es.glsl");
#else
    program = load_program("shaders/text_vertex.glsl", "shaders/text_fragment.glsl");
#endif
    text_attrib.program = program;
    text_attrib.position = 0;
    text_attrib.uv = 1;
    text_attrib.matrix = glGetUniformLocation(program, "matrix");
    text_attrib.sampler = glGetUniformLocation(program, "sampler");
    text_attrib.extra1 = glGetUniformLocation(program, "is_sign");

#if defined(__EMSCRIPTEN__)
    program = load_program("shaders/es/sky_vertex.es.glsl", "shaders/es/sky_fragment.es.glsl");
#else
    program = load_program("shaders/sky_vertex.glsl", "shaders/sky_fragment.glsl");
#endif
    sky_attrib.program = program;
    sky_attrib.position = 0;
    sky_attrib.normal = 1;
    sky_attrib.uv = 2;
    sky_attrib.matrix = glGetUniformLocation(program, "matrix");
    sky_attrib.sampler = glGetUniformLocation(program, "sampler");
    sky_attrib.timer = glGetUniformLocation(program, "timer");
    
    SDL_snprintf(m_pimpl->m_model->db_path, MAX_PATH_LENGTH, "%s", DB_PATH);

    m_pimpl->m_model->create_radius = CREATE_CHUNK_RADIUS;
    m_pimpl->m_model->render_radius = RENDER_CHUNK_RADIUS;
    m_pimpl->m_model->delete_radius = DELETE_CHUNK_RADIUS;
    m_pimpl->m_model->sign_radius = RENDER_SIGN_RADIUS;

    // INITIALIZE WORKER THREADS
    m_pimpl->init_worker_threads();

    // DEAR IMGUI INIT - Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup ImGui Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(m_pimpl->m_model->window, m_pimpl->m_model->context);
    string glsl_version = "";
#if defined(__EMSCRIPTEN__)
    glsl_version = "#version 100";
#else
    glsl_version = "#version 130";
#endif
    ImGui_ImplOpenGL3_Init(glsl_version.c_str());
    ImGui::StyleColorsLight();
    ImFont *nunito_sans_font = io.Fonts->AddFontFromMemoryCompressedTTF(NunitoSans_compressed_data, NunitoSans_compressed_size, 18.f);

#if defined(MAZE_DEBUG)
    IM_ASSERT(nunito_sans_font != nullptr);
#endif
    
    auto _check_for_gl_err = [](const char *file, int line) -> GLenum {
        GLenum errorCode;
        while ((errorCode = glGetError()) != GL_NO_ERROR) {
            std::string error = "";
            switch (errorCode) {
                case GL_INVALID_ENUM: error = "INVALID_ENUM"; break;
                case GL_INVALID_VALUE: error = "INVALID_VALUE"; break;
                case GL_INVALID_OPERATION: error = "INVALID_OPERATION"; break;
                case GL_OUT_OF_MEMORY: error = "OUT_OF_MEMORY"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
            }
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                "OpenGL ERROR: %s\n\t\tFILE: %s, LINE: %d\n", error.c_str(), file, line);
        }
        return errorCode;
    };
#define check_for_gl_err() _check_for_gl_err(__FILE__, __LINE__)

#if defined(MAZE_DEBUG)
    SDL_Log("check_for_gl_err() prior to the db init\n");
    check_for_gl_err();
#endif

    // DATABASE INITIALIZATION 
    if (USE_CACHE) {
        db_enable();
        if (db_init(m_pimpl->m_model->db_path)) {
            return false;
        }
    }

    // LOCAL VARIABLES 
    m_pimpl->reset_model();
    FPS fps = {0, 0, 0};
    uint64_t last_commit = SDL_GetTicks();

    GLuint sky_buffer = m_pimpl->gen_sky_buffer();

    craft_impl::Player *me = m_pimpl->m_model->players;
    craft_impl::State *p_state = &m_pimpl->m_model->players->state;
    me->id = 0;
    me->name[0] = '\0';
    me->buffer = 0;
    m_pimpl->m_model->player_count = 1;

    // magic variables to prevent black screen on load - modified in handle_events()
    this->m_pimpl->m_model->is_ortho = 0;
    this->m_pimpl->m_model->fov = 65;

    // LOAD STATE FROM DATABASE 
    int loaded = db_load_state(&p_state->x, &p_state->y, &p_state->z, &p_state->rx, &p_state->ry);

    m_pimpl->force_chunks(me);

    if (!loaded) {
        p_state->y = static_cast<float>(m_pimpl->highest_block(p_state->x, p_state->z) + 5);
    }

    // Init some local vars for handling maze duties
    auto my_maze_type = get_maze_algo_from_str(algos.back());
    auto&& gui = this->m_pimpl->m_gui;
    auto&& maze2 = this->m_pimpl->m_maze;

    auto make_maze_ptr = [&my_maze_type, &get_int, &rng_machine, &maze2](unsigned int w, unsigned int l, unsigned int h) {
        maze2 = std::make_unique<maze_thread_safe>(my_maze_type, get_int, rng_machine, w, l, h);
    };

    // Generate a default maze to start the app
    future<void> maze_gen_future = async(launch::async, make_maze_ptr, gui->maze_width, gui->maze_length, gui->maze_height);

    auto progress_tracker = std::make_shared<craft::craft_impl::ProgressTracker>();
    
    future<bool> write_success;
    auto maze_writer_fut = [&maze2](const string& filename) {
        if (!filename.empty()) {
            // get ready to write to file
            packaged_task<bool(const string& out_file)> maze_writing_task{ [&maze2](auto out)->bool {
                writer maze_writer;
                return maze_writer.write(out, maze2->to_wavefront_obj_str());
            }};
            auto writing_results = maze_writing_task.get_future();
            thread(std::move(maze_writing_task), filename).detach();
            return writing_results;
        } else {
            promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }
	};

    auto json_writer = [&maze2](const string& outfile) -> string {
        auto&& vertices = maze2->get_writable_vertices();
        auto&& faces = maze2->get_faces();
        stringstream ss;
        // Set key if outfile is specified
        ss << "{\"name\":\"" << outfile << "\", \"data\":[";
        // Wavefront object file header
        ss << "\"# https://www.github.com/zmertens/MazeBuilder\\n\"";
        for (const auto& vertex : vertices) {
            ss << ",\"v " << get<0>(vertex) << " " << get<1>(vertex) << " " << get<2>(vertex) << "\\n\"";
        }

        // Note in writing the face that the index is 1-based
        // Also, there is no space after the 'f', until the loop
        for (const auto& face : faces) {
            ss << ",\"f";
            for (auto index : face) {
                ss << " " << index;
            }
            ss << "\\n\"";
        }

        ss << "]}";

        return ss.str();
    };

#if defined(MAZE_DEBUG)
    SDL_Log("check_for_gl_err() prior to event loop\n");
    check_for_gl_err();
#endif

    int triangle_faces = 0;
    bool running = true;

    auto is_click_inside_gui = [](float m_x, float m_y, float gui_pos_x, float gui_pos_y, float gui_width, float gui_height) {
        return (m_x >= gui_pos_x) && (m_x < (gui_pos_x + gui_width)) && (m_y >= gui_pos_y) && (m_y < (gui_pos_y + gui_height));
    };
    uint64_t previous = SDL_GetTicks();
    // BEGIN EVENT LOOP
#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (running)
#endif
    {
        glViewport(0, 0, this->m_pimpl->m_model->width, this->m_pimpl->m_model->height);
        // FRAME RATE 
        if (m_pimpl->m_model->time_changed) {
            m_pimpl->m_model->time_changed = 0;
            last_commit = SDL_GetTicks();
            SDL_memset(&fps, 0, sizeof(fps));
        }
        update_fps(&fps);
        uint64_t now = SDL_GetTicks();
        double dt = static_cast<double>(now - previous) / 1000.0;
        dt = SDL_min(dt, 0.2);
        dt = SDL_max(dt, 0.0);
        previous = now;

        // ImGui window state variables
        static bool show_demo_window = false;
        static bool show_mb_gui = !SDL_GetWindowRelativeMouseMode(this->m_pimpl->m_model->window);
        static bool write_maze_now = false;
        static bool first_maze = true;
        // Handle SDL events
        bool events_handled_success = m_pimpl->handle_events(dt, ref(running));

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

#if defined(MAZE_DEBUG)
        if (ImGui::IsMousePosValid()) {
            float m_x = io.MousePos.x;
            float m_y = io.MousePos.y;
            ImGui::Text("Mouse pos: (%g, %g)", m_x, m_y);
            ImVec2 window_pos = ImGui::GetWindowPos();
            ImVec2 window_size = ImGui::GetWindowSize();
            bool is_gui_click = is_click_inside_gui(m_x, m_y, window_pos.x, window_pos.y, window_size.x, window_size.y);
            ImGui::Text("is_gui_click: %s", is_gui_click ? "true" : "false");
        } else {
            ImGui::Text("Mouse pos: <INVALID>");
        }
#endif 

        // Show the big demo window?
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        show_mb_gui = !SDL_GetWindowRelativeMouseMode(this->m_pimpl->m_model->window);
        // Maze Builder GUI
        if (show_mb_gui) {
            ImGui::PushFont(nunito_sans_font);
            // GUI Title Bar
            ImGui::Begin(this->m_pimpl->m_version.data());

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

            // Set the time
            int hour = static_cast<int>(m_pimpl->time_of_day() * 24.f);
            char am_pm = hour < 12 ? 'a' : 'p';
            hour = hour % 12;
            hour = hour ? hour : 12;
            ImGui::Text("chunk.p: %d, chunk.q: %d", m_pimpl->chunked(p_state->x), m_pimpl->chunked(p_state->z));
            ImGui::Text("player.x: %.2f, player.y: %.2f, player.z: %.2f", p_state->x, p_state->y, p_state->z);
            ImGui::Text("#chunks: %d, #triangles: %d", m_pimpl->m_model->chunk_count, triangle_faces * 2);
            ImGui::Text("time: %d%cm", hour, am_pm);

            // GUI Tabs
            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {

                if (ImGui::BeginTabItem("Builder")) {
                    ImGui::Text("Builder settings");

                    static unsigned int MAX_MAZE_WIDTH = 1000;
                    if (ImGui::SliderInt("Width", &gui->maze_width, 25, MAX_MAZE_WIDTH)) {

                    }
                    static unsigned int MAX_MAZE_LENGTH = 1000;
                    if (ImGui::SliderInt("Length", &gui->maze_length, 25, MAX_MAZE_LENGTH)) {

                    }
                    static unsigned int MAX_MAZE_HEIGHT = 15;
                    if (ImGui::SliderInt("Height", &gui->maze_height, 1, MAX_MAZE_HEIGHT)) {
                      
                    }
                    static unsigned int MAX_SEED_VAL = 1'000;
                    if (ImGui::SliderInt("Seed", &gui->seed, 0, MAX_SEED_VAL)) {
                        rng_machine.seed(static_cast<unsigned long>(gui->seed));
                    }
                    ImGui::InputText("Outfile", &gui->outfile[0], IM_ARRAYSIZE(gui->outfile));
                    if (ImGui::TreeNode("Maze Generator")) {
                        auto preview{ gui->maze_algo.c_str() };
                        ImGui::NewLine();
                        ImGuiComboFlags combo_flags = ImGuiComboFlags_PopupAlignLeft;
                        if (ImGui::BeginCombo("algorithm", preview, combo_flags)) {
                            for (const auto& itr : algos) {
                                bool is_selected = (itr == gui->maze_algo);
                                if (ImGui::Selectable(itr.c_str(), is_selected)) {
                                    gui->maze_algo = itr;
                                    my_maze_type = get_maze_algo_from_str(itr);
                                }
                                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::NewLine();
                        ImGui::TreePop();
                    }

                    // Check if user has added a prefix to the Wavefront object file
                    if (gui->outfile[0] != '.') {
                        if (ImGui::Button("Build!")) {
                            progress_tracker->start();
                            // Start the maze generation in the background
                            maze_gen_future = async(launch::async, make_maze_ptr, gui->maze_width, gui->maze_length, gui->maze_height);
                            progress_tracker->stop();
                            // Hack to force the chunks to load, will reset the player's position next loop
                            p_state->y = 1000.f;
                            p_state->x = 1000.f;
                            p_state->z = 1000.f;
                        } else {
                            ImGui::SameLine();
                            ImGui::Text("Building maze... %s\n", gui->outfile);
                        }
                    } else {
                        // Disable the button
                        ImGui::BeginDisabled(true);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha | ImGuiTabItemFlags_None, ImGui::GetStyle().Alpha * 0.5f);

                        // Render the button - don't need to check if the button is pressed because it's disabled
                        ImGui::Button("Build!");

                        // Re-enable items and revert style change
                        ImGui::PopStyleVar();
                        ImGui::EndDisabled();
                    }

                    // Let JavaScript handle file downloads in web browser
#if !defined(__EMSCRIPTEN)  
                    if (write_success.valid() && write_success.wait_for(chrono::seconds(0)) == future_status::ready) {
                        // Call the writer future and get the result
                        bool success = write_success.get();
                        if (success && gui->outfile[0] != '.') {
                            // Dont display a message on the web browser, let the web browser handle that
                            ImGui::NewLine();
                            ImGui::Text("Maze written to %s\n", gui->outfile);
                            ImGui::NewLine();
                        } else {
                            ImGui::NewLine();
                            ImGui::Text("Failed to write maze: %s\n", gui->outfile);
                            ImGui::NewLine();
                        }
                        // Reset outfile's first char and that will disable the Build! button
                        gui->outfile[0] = '.';
                    }
#endif
                    if (progress_tracker) {
                        // Show progress when writing
                        ImGui::NewLine();
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.008f, 0.83f, 0.015f, 1.0f));
                        ImGui::Text("Finished building maze in %f ms", progress_tracker->get_duration_in_ms());
                        ImGui::NewLine();
                        ImGui::PopStyleColor();
                    }

                    // Reset should remove outfile name, clear vertex data for all generated mazes and remove them from the world
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.023f, 0.015f, 1.0f));
                    if (ImGui::Button("Reset")) {
                        // Clear the GUI
                    }
                    ImGui::PopStyleColor();
                    
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Graphics")) {
                    ImGui::Text("Graphic settings");
                    
                    ImGui::Checkbox("Dark Mode", &gui->color_mode_dark);
                    if (gui->color_mode_dark)
                        ImGui::StyleColorsDark();
                    else
                        ImGui::StyleColorsLight();
                    
                    // Prevent setting SDL_Window settings every frame
                    static bool last_fullscreen = gui->fullscreen;
                    ImGui::Checkbox("Fullscreen (ESC to Exit)", &gui->fullscreen);
                    bool update_fullscreen = (last_fullscreen != gui->fullscreen) ? true : false;
                    last_fullscreen = gui->fullscreen;
                    if (update_fullscreen)
                        SDL_SetWindowFullscreen(this->m_pimpl->m_model->window, gui->fullscreen);

                    ImGui::Checkbox("Capture Mouse (ESC to Uncapture)", &gui->capture_mouse);
                    if (gui->capture_mouse) {
                        SDL_SetWindowRelativeMouseMode(this->m_pimpl->m_model->window, SDL_TRUE);
                    } else {
                        SDL_SetWindowRelativeMouseMode(this->m_pimpl->m_model->window, SDL_FALSE);
                    }

                    static bool last_vsync = gui->vsync;
                    ImGui::Checkbox("VSYNC", &gui->vsync);
                    bool update_vsync = (last_vsync != gui->vsync) ? true : false;
                    last_vsync = gui->vsync;
                    if (update_vsync)
                        SDL_GL_SetSwapInterval(gui->vsync);

                    ImGui::Checkbox("Show Lights", &gui->show_lights);
                    ImGui::Checkbox("Show Items", &gui->show_items);
                    ImGui::Checkbox("Show Wireframes", &gui->show_wireframes);
                    ImGui::Checkbox("Show Crosshairs", &gui->show_crosshairs);

                                    
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Help")) {
                    ImGui::Text("%s\n", this->m_pimpl->m_help.data());
                    static constexpr auto github_repo = R"gh(https://github.com/zmertens/MazeBuilder)gh";
                    ImGui::Text("\n");
                    ImGui::Text(github_repo);
                    ImGui::Text("\n");
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            } // imgui tab handler
            ImGui::End();
            ImGui::PopFont();
        } // show_builder_gui

        // Check if maze is available and then perform two async operations:
        // 1. Set maze string and compute maze geometry for 3D coordinates (includes a height value)
        // 2. Write the maze to a Wavefront object file using the computed data (except the default maze)
        if (maze_gen_future.valid() && maze_gen_future.wait_for(chrono::seconds(0)) == future_status::ready) {
            // Reset player state to roughly the origin
            p_state->y = 10.f;
            p_state->x = 0.f;
            p_state->z = 0.f;
            // Look in +x, +y direction
            p_state->rx = 100;
            p_state->ry = 100;
            // Get the maze and reset the future
            maze_gen_future.get();
            // Don't write the first maze that loads when app starts
            write_maze_now = first_maze ? false : true;
            first_maze = false;
        }

        if (write_maze_now) {
            // Writing the maze will run in the background - only do that on Desktop
            write_maze_now = false;
            // Only write the maze when **NOT** on the web browser
#if !defined(__EMSCRIPTEN__ )
            write_success = maze_writer_fut(gui->outfile);
#endif
            this->m_pimpl->m_gui->maze_json = json_writer(gui->outfile);
        } else {
            // Failed to set maze
        }

        // FLUSH DATABASE 
        if (now - last_commit > COMMIT_INTERVAL) {
            last_commit = now;
            db_commit();
        }
    
        craft_impl::Player* player = m_pimpl->m_model->players + m_pimpl->m_model->observe1;

        // PREPARE TO RENDER 
        m_pimpl->m_model->observe1 = m_pimpl->m_model->observe1 % m_pimpl->m_model->player_count;
        m_pimpl->m_model->observe2 = m_pimpl->m_model->observe2 % m_pimpl->m_model->player_count;
    
        m_pimpl->delete_chunks();
        m_pimpl->del_buffer(me->buffer);
    
        me->buffer = m_pimpl->gen_player_buffer(p_state->x, p_state->y, p_state->z, p_state->rx, p_state->ry);
        for (int i = 1; i < m_pimpl->m_model->player_count; i++) {
            m_pimpl->interpolate_player(m_pimpl->m_model->players + i);
        }

        // RENDER 3-D SCENE
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_pimpl->render_sky(&sky_attrib, player, sky_buffer);
        glClear(GL_DEPTH_BUFFER_BIT);

        triangle_faces = m_pimpl->render_chunks(&block_attrib, player);
        m_pimpl->render_signs(&text_attrib, player);
        m_pimpl->render_sign(&text_attrib, player);
        m_pimpl->render_players(&block_attrib, player);
        if (gui->show_wireframes) {
            m_pimpl->render_wireframe(&line_attrib, player);
        }

        // RENDER HUD 
        glClear(GL_DEPTH_BUFFER_BIT);
        if (gui->show_crosshairs) {
            m_pimpl->render_crosshairs(&line_attrib);
        }
        if (gui->show_items) {
            m_pimpl->render_item(&block_attrib);
        }

        // RENDER TEXT 
        char text_buffer[1024];
        float ts = static_cast<float>(12 * m_pimpl->m_model->scale);
        float tx = ts / 2.f;
        float ty = m_pimpl->m_model->height - ts;
        if (SHOW_CHAT_TEXT) {
            for (int i = 0; i < MAX_MESSAGES; i++) {
                int index = (m_pimpl->m_model->message_index + i) % MAX_MESSAGES;
                if (SDL_strlen(m_pimpl->m_model->messages[index])) {
                    m_pimpl->render_text(&text_attrib, ALIGN_LEFT, tx, ty, ts,
                        m_pimpl->m_model->messages[index]);
                    ty -= ts * 2;
                }
            }
        }
        if (m_pimpl->m_model->typing) {
            SDL_snprintf(text_buffer, 1024, "> %s", m_pimpl->m_model->typing_buffer);
            m_pimpl->render_text(&text_attrib, ALIGN_LEFT, tx, ty, ts, text_buffer);
        }
        if (SHOW_PLAYER_NAMES) {
            if (player != me) {
                m_pimpl->render_text(&text_attrib, ALIGN_CENTER, static_cast<float>(m_pimpl->m_model->width) / 2.f, ts, ts, player->name);
            }
            craft_impl::Player* other = m_pimpl->player_crosshair(player);
            if (other) {
                m_pimpl->render_text(&text_attrib, ALIGN_CENTER, static_cast<float>(m_pimpl->m_model->width) / 2.f, static_cast<float>(m_pimpl->m_model->height) / 2.f - ts - 24.f, ts, other->name);
            }
        }

        // RENDER PICTURE IN PICTURE 
        if (m_pimpl->m_model->observe2) {
            player = m_pimpl->m_model->players + m_pimpl->m_model->observe2;

            int pw = 256 * m_pimpl->m_model->scale;
            int ph = 256 * m_pimpl->m_model->scale;
            int offset = 32 * m_pimpl->m_model->scale;
            int pad = 3 * m_pimpl->m_model->scale;
            int sw = pw + pad * 2;
            int sh = ph + pad * 2;

            glEnable(GL_SCISSOR_TEST);
            glScissor(m_pimpl->m_model->width - sw - offset + pad, offset - pad, sw, sh);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_SCISSOR_TEST);
            glClear(GL_DEPTH_BUFFER_BIT);
            glViewport(m_pimpl->m_model->width - pw - offset, offset, pw, ph);

            m_pimpl->m_model->width = pw;
            m_pimpl->m_model->height = ph;
            m_pimpl->m_model->is_ortho = false;
            m_pimpl->m_model->fov = 65;

            m_pimpl->render_sky(&sky_attrib, player, sky_buffer);

            glClear(GL_DEPTH_BUFFER_BIT);
            m_pimpl->render_chunks(&block_attrib, player);

            m_pimpl->render_signs(&text_attrib, player);

            m_pimpl->render_players(&block_attrib, player);

            glClear(GL_DEPTH_BUFFER_BIT);

            if (SHOW_PLAYER_NAMES) {
                m_pimpl->render_text(&text_attrib, ALIGN_CENTER, static_cast<float>(pw) / 2.f, ts, ts, player->name);
            }
        } // render picture in picture
            
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(m_pimpl->m_model->window);

#if defined(MAZE_DEBUG)
            check_for_gl_err();
#endif

        if (!events_handled_success || !running) {
#if defined(__EMSCRIPTEN__)
            emscripten_cancel_main_loop();
#endif
        }
    } // EVENT LOOP

#if defined(__EMSCRIPTEN__)
        EMSCRIPTEN_MAINLOOP_END;
#endif

#if defined(MAZE_DEBUG)
    SDL_Log("Cleaning up ImGui objects. . .");
    SDL_Log("Cleaning up OpenGL objects. . .");
    SDL_Log("Cleaning up SDL objects. . .");
#endif

    m_pimpl->cleanup_worker_threads();

#if defined(MAZE_DEBUG)
    SDL_Log("Closing DB. . .\n");
#endif

    db_save_state(p_state->x, p_state->y, p_state->z, p_state->rx, p_state->ry);
    db_close();
    db_disable();

#if defined(MAZE_DEBUG)
    SDL_Log("Deleting buffer objects. . .");
#endif
    m_pimpl->del_buffer(sky_buffer);
    m_pimpl->delete_all_chunks();
    m_pimpl->delete_all_players();
#if defined(MAZE_DEBUG)
    SDL_Log("check_for_gl_err() at the end of the event loop\n");
    check_for_gl_err();
#endif

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    glDeleteTextures(1, &texture);
    glDeleteTextures(1, &font);
    glDeleteTextures(1, &sky);
    glDeleteTextures(1, &sign);
    glDeleteProgram(block_attrib.program);
    glDeleteProgram(text_attrib.program);
    glDeleteProgram(sky_attrib.program);
    glDeleteProgram(line_attrib.program);

    SDL_GL_DestroyContext(m_pimpl->m_model->context);
    SDL_DestroyWindow(m_pimpl->m_model->window);
    SDL_Quit();

    return true;
}  // run

void craft::set_json(const string& s) noexcept {
    this->m_pimpl->m_gui->maze_json = s;
}

/**
 * 
 * 
 * @brief Used by Emscripten mostly to produce a JSON string containing the vertex data
 * @return returns JSON-encoded string: "{\"name\":\"MyMaze\", \"data\":\"v 1.0 1.0 0.0\\nv -1.0 1.0 0.0\\n...\"}";
 */
std::string craft::get_json() const noexcept {
    return this->m_pimpl->m_gui->maze_json;
}
