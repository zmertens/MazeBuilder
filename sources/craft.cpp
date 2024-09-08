/*
 * Craft engine builds voxels as chunks and can run maze-generating algorithms
 * Generated mazes are stored in-memory and in an offline database
 * Vertex and indice data is stored in buffers and rendered using OpenGL
 * Supports RESTful APIs for web applications by passing maze data in JSON format
 * Interfaces with Emscripten to provide web API support
 * 
 * 
 * Originally written in C99, ported to C++17
*/

#include "craft.h"

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#include <emscripten_local/emscripten_mainloop_stub.h>
#endif

#define RAYGUI_IMPLEMENTATION
#include <raylib/raygui.h>
#include <raylib/rlgl.h>
#include <raylib/glad.h>
#include <raylib/raymath.h>

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

#include "world.h"
#include "db.h"
#include "item.h"

#include "maze_types_enum.h"
#include "maze_factory.h"
#include "maze_thread_safe.h"
#include "grid.h"
#include "cell.h"
#include "writer.h"

// Movement configurations
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

// Typing config
#define CRAFT_KEY_SIGN '`'

// World configs
#define INIT_WINDOW_WIDTH 1024
#define INIT_WINDOW_HEIGHT 768
#define SCROLL_THRESHOLD 0.1
#define DB_PATH "craft.db"
#define MAX_DB_PATH_LEN 64
#define USE_CACHE true
#define DAY_LENGTH 600
#define INVERT_MOUSE 0
#define MAX_TEXT_LENGTH 256

// Advanced options
#define CREATE_CHUNK_RADIUS 10
#define RENDER_CHUNK_RADIUS 20
#define RENDER_SIGN_RADIUS 4
#define DELETE_CHUNK_RADIUS 14
#define COMMIT_INTERVAL 5
#define MAX_CHUNKS 8192
#define NUM_WORKERS 4

#define WORKER_IDLE 0
#define WORKER_BUSY 1
#define WORKER_DONE 2

using namespace mazes;
using namespace std;

class craft::craft_impl {
public:
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
            outfile(".obj"), seed(101), maze_width(25), maze_height(5), maze_length(28),
            maze_algo("binary_tree"), maze_json("") {
        
        }

        void reset_outfile() {
            for (auto i = 0; i < sizeof(outfile) / sizeof(char); ++i) {
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

    class ProgressTracker {
    public:
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

    class Chunk {
    public:
        Map map;
        Map lights;
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

    class WorkerItem {
    public:
        int p;
        int q;
        int load;
        Map *block_maps[3][3];
        Map *light_maps[3][3];
        int miny;
        int maxy;
        int faces;
        float *data;
    };

    class Worker {
    public:
        int index;
        int state;
        std::thread thrd;
        std::mutex mtx;
        std::condition_variable cnd;
        WorkerItem item;
        bool should_stop;
    };

    class Block {
    public:
        int x;
        int y;
        int z;
        int w;
    };

    class State {
    public:
        float x;
        float y;
        float z;
        float rx;
        float ry;
        float t;
    };

    class Player {
    public:
        int id;
        std::string name;
        State state;
        State state1;
        State state2;
        std::uint32_t buffer;
    };

    class Attrib {
    public:
        Shader shader;
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
    };

    class Model {
    public:
        std::vector<std::unique_ptr<Worker>> workers;
        Chunk chunks[MAX_CHUNKS];
        int chunk_count;
        int create_radius;
        int render_radius;
        int delete_radius;
        int sign_radius;
        Player players[MAX_PLAYERS];
        int player_count;
        int width;
        int height;
        bool flying;
        int item_index;
        int scale;
        bool is_ortho;
        float fov;
        int suppress_char;
        int mode_changed;
        char db_path[MAX_DB_PATH_LEN];
        bool typing;
        char typing_buffer[MAX_TEXT_LENGTH];
        size_t text_len;
        int day_length;
        bool time_changed;
        int start_time;
        Block block0;
        Block block1;
        Block copy0;
        Block copy1;
        int window_width;
        int window_height;
    };

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
        , m_model{ make_unique<Model>() }
        , m_maze()
        , m_gui{make_unique<Gui>()} {
        this->reset_model();
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
            cout << "INFO: Worker thread " << w->index << " finished!" << endl;
#endif
        }
        // Clear the vector after all threads have been joined
        this->m_model->workers.clear();
    }

    void del_buffer(std::uint32_t buffer) const {
        glDeleteBuffers(1, &buffer);
    }

    std::uint32_t gen_buffer(std::size_t size, float *data) const {
        std::uint32_t buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return buffer;
    }

    float *malloc_faces(std::size_t components, std::size_t faces) const {
        return (float*) malloc(sizeof(float) * 6 * components * faces);;
    }

    /**
     * @brief Generate a buffer for faces - data is not freed here
     */
    std::uint32_t gen_faces(GLsizei components, GLsizei faces, float *data) const {
        return this->gen_buffer(sizeof(float) * 6 * components * faces, data);
    }

    int chunked(float x) const {
        return static_cast<int>((roundf(x) / this->m_gui->chunk_size));
    }

    double get_time() const {
    	return (GetFrameTime() + static_cast<float>(this->m_model->start_time)) / 1000.0f;
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
            return 1 / (1 + powf(2.f, -t));
        }
        else {
            float t = (timer - 0.85f) * 100.f;
            return 1.f - 1.f / (1.f + powf(2.f, -t));
        }
    }

    int get_scale_factor() const {
        return static_cast<int>(GetWindowScaleDPI().x / GetWindowScaleDPI().y);
    }

    void get_sight_vector(float rx, float ry, float *vx, float *vy, float *vz) const {
        float m = cosf(ry);
        *vx = cosf(rx - static_cast<float>(PI / 180.f * 90.f)) * m;
        *vy = sinf(ry);
        *vz = sinf(rx - static_cast<float>(PI / 180.f * 90.f)) * m;
    }

    void get_motion_vector(int flying, int sz, int sx, float rx, float ry,
        float *vx, float *vy, float *vz) const {
        *vx = 0; *vy = 0; *vz = 0;
        if (!sz && !sx) {
            return;
        }
        float strafe = atan2f(static_cast<float>(sz), static_cast<float>(sx));
        if (flying) {
            float m = cosf(ry);
            float y = sinf(ry);
            if (sx) {
                if (!sz) {
                    y = 0;
                }
                m = 1;
            }
            if (sz > 0) {
                y = -y;
            }
            *vx = cosf(rx + strafe) * m;
            *vy = y;
            *vz = sinf(rx + strafe) * m;
        }
        else {
            *vx = cosf(rx + strafe);
            *vy = 0;
            *vz = sinf(rx + strafe);
        }
    }

    //std::uint32_t gen_crosshair_buffer() {
    //    float x = static_cast<float>(this->m_model->width) / 2.0f;
    //    float y = static_cast<float>(this->m_model->height) / 2.0f;
    //    float p = 10.f * static_cast<float>(this->m_model->scale);
    //    float data[] = {
    //        x, y - p, x, y + p,
    //        x - p, y, x + p, y
    //    };
    //    return this->gen_buffer(sizeof(data), data);
    //}

    std::uint32_t gen_wireframe_buffer(float x, float y, float z, float n) {
    //    float data[72];
    //    // cube.h -> make_cube_wireframe
    //    make_cube_wireframe(data, x, y, z, n);
    //    return this->gen_buffer(sizeof(data), data);
    }

    std::uint32_t gen_cube_buffer(float x, float y, float z, float n, int w) {
    //    float *data = malloc_faces(10, 6);
    //    float ao[6][4] = {0};
    //    float light[6][4] = {
    //        {0.5, 0.5, 0.5, 0.5},
    //        {0.5, 0.5, 0.5, 0.5},
    //        {0.5, 0.5, 0.5, 0.5},
    //        {0.5, 0.5, 0.5, 0.5},
    //        {0.5, 0.5, 0.5, 0.5},
    //        {0.5, 0.5, 0.5, 0.5}
    //    };
    //    make_cube(data, ao, light, 1, 1, 1, 1, 1, 1, x, y, z, n, w);
    //    return gen_faces(10, 6, data);
        return 0;
    }

    std::uint32_t gen_plant_buffer(float x, float y, float z, float n, int w) {
    //    float *data = malloc_faces(10, 4);
    //    float ao = 0;
    //    float light = 1;
    //    make_plant(data, ao, light, x, y, z, n, w, 45);
    //    return gen_faces(10, 4, data);
        return 0;
    }

    std::uint32_t gen_player_buffer(float x, float y, float z, float rx, float ry) {
    //    float *data = malloc_faces(10, 6);
    //    make_player(data, x, y, z, rx, ry);
    //    return gen_faces(10, 6, data);
        return 0;
    }

    std::uint32_t gen_text_buffer(float x, float y, float n, char *text) {
    //    GLsizei length = static_cast<GLsizei>(strlen(text));
    //    float *data = malloc_faces(4, length);
    //    for (int i = 0; i < length; i++) {
    //        make_character(data + i * 24, x, y, n / 2, n, text[i]);
    //        x += n;
    //    }
    //    return gen_faces(4, length, data);
        return 0;
    }

    void draw_triangles_3d_ao(Attrib *attrib, std::uint32_t buffer, int count) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glEnableVertexAttribArray(attrib->position);
        glEnableVertexAttribArray(attrib->normal);
        glEnableVertexAttribArray(attrib->uv);
        glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
            sizeof(float) * 10, 0);
        glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE,
            sizeof(float) * 10, (GLvoid *)(sizeof(float) * 3));
        glVertexAttribPointer(attrib->uv, 4, GL_FLOAT, GL_FALSE,
            sizeof(float) * 10, (GLvoid *)(sizeof(float) * 6));
        glDrawArrays(GL_TRIANGLES, 0, count);
        glDisableVertexAttribArray(attrib->position);
        glDisableVertexAttribArray(attrib->normal);
        glDisableVertexAttribArray(attrib->uv);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_triangles_3d_text(Attrib *attrib, std::uint32_t buffer, int count) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glEnableVertexAttribArray(attrib->position);
        glEnableVertexAttribArray(attrib->uv);
        glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
            sizeof(float) * 5, 0);
        glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
            sizeof(float) * 5, (GLvoid *)(sizeof(float) * 3));
        glDrawArrays(GL_TRIANGLES, 0, count);
        glDisableVertexAttribArray(attrib->position);
        glDisableVertexAttribArray(attrib->uv);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_triangles_3d(Attrib *attrib, GLuint buffer, int count) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);

        glEnableVertexAttribArray(attrib->position);

        glEnableVertexAttribArray(attrib->normal);
        glEnableVertexAttribArray(attrib->uv);

        glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
        glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid *)(sizeof(float) * 3));
        glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid *)(sizeof(float) * 6));
        
        glDrawArrays(GL_TRIANGLES, 0, count);

        glDisableVertexAttribArray(attrib->position);
        glDisableVertexAttribArray(attrib->normal);
        glDisableVertexAttribArray(attrib->uv);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_triangles_2d(Attrib *attrib, std::uint32_t buffer, std::size_t count) {
    //    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    //    glEnableVertexAttribArray(attrib->position);
    //    glEnableVertexAttribArray(attrib->uv);
    //    glVertexAttribPointer(attrib->position, 2, GL_FLOAT, GL_FALSE,
    //        sizeof(float) * 4, 0);
    //    glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
    //        sizeof(float) * 4, (GLvoid *)(sizeof(float) * 2));
    //    glDrawArrays(GL_TRIANGLES, 0, count);
    //    glDisableVertexAttribArray(attrib->position);
    //    glDisableVertexAttribArray(attrib->uv);
    //    glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_lines(Attrib *attrib, std::uint32_t buffer, int components, int count) {
    //    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    //    glEnableVertexAttribArray(attrib->position);
    //    glVertexAttribPointer(
    //        attrib->position, components, GL_FLOAT, GL_FALSE, 0, 0);
    //    glDrawArrays(GL_LINES, 0, count);
    //    glDisableVertexAttribArray(attrib->position);
    //    glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_chunk(Attrib *attrib, Chunk *chunk) {
    //    draw_triangles_3d_ao(attrib, chunk->buffer, chunk->faces * 6);
    }

    void draw_item(Attrib *attrib, std::uint32_t buffer, int count) {
    //    draw_triangles_3d_ao(attrib, buffer, count);
    }

    void draw_text(Attrib *attrib, std::uint32_t buffer, std::size_t length) {
    //    glEnable(GL_BLEND);
    //    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //    draw_triangles_2d(attrib, buffer, length * 6);
    //    glDisable(GL_BLEND);
    }

    void draw_signs(Attrib *attrib, Chunk *chunk) {
    //    glEnable(GL_POLYGON_OFFSET_FILL);
    //    glPolygonOffset(-8, -1024);
    //    draw_triangles_3d_text(attrib, chunk->sign_buffer, chunk->sign_faces * 6);
    //    glDisable(GL_POLYGON_OFFSET_FILL);
    }

    void draw_sign(Attrib *attrib, std::uint32_t buffer, int length) {
    //    glEnable(GL_POLYGON_OFFSET_FILL);
    //    glPolygonOffset(-8, -1024);
    //    draw_triangles_3d_text(attrib, buffer, length * 6);
    //    glDisable(GL_POLYGON_OFFSET_FILL);
    }

    void draw_cube(Attrib *attrib, std::uint32_t buffer) {
    //    draw_item(attrib, buffer, 36);
    }

    void draw_plant(Attrib *attrib, std::uint32_t buffer) {
    //    draw_item(attrib, buffer, 24);
    }

    void draw_player(Attrib *attrib, Player *player) {
    //    draw_cube(attrib, player->buffer);
    }

    void update_player(Player *player, float x, float y, float z, float rx, float ry, int interpolate)
    {
        if (interpolate) {
            State *s1 = &player->state1;
            State *s2 = &player->state2;
            memcpy(s1, s2, sizeof(State));
            s2->x = x; s2->y = y; s2->z = z; s2->rx = rx; s2->ry = ry;
            s2->t = static_cast<float>(get_time());
            if (s2->rx - s1->rx > static_cast<float>(M_PI)) {
                s1->rx += static_cast<float>(2 * M_PI);
            }
            if (s1->rx - s2->rx > static_cast<float>(M_PI)) {
                s1->rx -= static_cast<float>(2 * M_PI);
            }
        }
        //else {
        //    State *s = &player->state;
        //    s->x = x; s->y = y; s->z = z; s->rx = rx; s->ry = ry;
        //    del_buffer(player->buffer);
        //    player->buffer = gen_player_buffer(s->x, s->y, s->z, s->rx, s->ry);
        //}
    }

    void interpolate_player(Player *player) {
        State *s1 = &player->state1;
        State *s2 = &player->state2;
        float t1 = static_cast<float>(s2->t - s1->t);
        float t2 = static_cast<float>(this->get_time()) - s2->t;
        t1 = min(t1, 1.f);
        t1 = max(t1, 0.1f);
        float p = min(t2 / t1, 1.f);
        this->update_player(
            player,
            s1->x + (s2->x - s1->x) * p,
            s1->y + (s2->y - s1->y) * p,
            s1->z + (s2->z - s1->z) * p,
            s1->rx + (s2->rx - s1->rx) * p,
            s1->ry + (s2->ry - s1->ry) * p,
            0);
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
        int dp = abs(chunk->p - p);
        int dq = abs(chunk->q - q);
        return max(dp, dq);
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
        int nx = static_cast<int>(roundf(x));
        int nz = static_cast<int>(roundf(z));
        int p = this->chunked(x);
        int q = this->chunked(z);
        Chunk *chunk = this->find_chunk(p, q);
        if (chunk) {
            Map *map = &chunk->map;
            MAP_FOR_EACH(map, ex, ey, ez, ew) {
                // item.h -> is_obstacle
                if (is_obstacle(ew) && ex == nx && ez == nz) {
                    result = max(result, ey);
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
            long nx = lroundf(x);
            long ny = lroundf(y);
            long nz = lroundf(z);
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
                float d = sqrtf(powf(hx - x, 2) + powf(hy - y, 2) + powf(hz - z, 2));
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
                float degrees = roundf(static_cast<float>(180.f / PI * atan2(s->x - hx, s->z - hz)));
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
        int nx = static_cast<int>(roundf(*x));
        int ny = static_cast<int>(roundf(*y));
        int nz = static_cast<int>(roundf(*z));
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
        int nx = static_cast<int>(roundf(x));
        int ny = static_cast<int>(roundf(y));
        int nz = static_cast<int>(roundf(z));
        for (int i = 0; i < height; i++) {
            if (nx == hx && ny - i == hy && nz == hz) {
                return 1;
            }
        }
        return 0;
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
                ao[i][j] = min(total, 1.0f);
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
        char *opaque = (char *)calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
        char *light = (char *)calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
        char *highest = (char *)calloc(XZ_SIZE * XZ_SIZE, sizeof(char));

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
                        highest[XZ(x, z)] = max(static_cast<int>(highest[XZ(x, z)]), y);
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
            miny = min(miny, ey);
            maxy = max(maxy, ey);
            faces += total;
        } END_MAP_FOR_EACH;

        // generate geometry
        // each vertex has 10 components (x, y, z, nx, ny, nz, u, v, ao, light)
        static constexpr int components = 10;
        float *data = malloc_faces(components, faces);
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
                        min_ao = min(min_ao, ao[a][b]);
                        max_light = max(max_light, light[a][b]);
                    }
                }
                float rotation = simplex2(static_cast<float>(ex), static_cast<float>(ez), 4, 0.5f, 2.f) * 360.f;
                /*make_plant(
                    data + offset, min_ao, max_light,
                    static_cast<float>(ex), static_cast<float>(ey), static_cast<float>(ez),
                    0.5f, ew, rotation);*/
            }
            else {
                //make_cube(
                //    data + offset, ao, light,
                //    f1, f2, f3, f4, f5, f6,
                //    static_cast<float>(ex), static_cast<float>(ey), static_cast<float>(ez), 0.5f, ew);
            }
            offset += total * 60;
        } END_MAP_FOR_EACH;

        free(opaque);
        free(light);
        free(highest);

        item->miny = miny;
        item->maxy = maxy;
        item->faces = faces;
        item->data = data;
    } // compute_chunk

    void generate_chunk(Chunk *chunk, WorkerItem *item) const {
    //    chunk->miny = item->miny;
    //    chunk->maxy = item->maxy;
    //    chunk->faces = item->faces;
    //    this->del_buffer(chunk->buffer);
    //    chunk->buffer = this->gen_faces(10, item->faces, item->data);
    //    this->gen_sign_buffer(chunk);
    }

    void gen_chunk_buffer(Chunk *chunk) const {
        WorkerItem _item;
        WorkerItem *item = &_item;
        //item->p = chunk->p;
        //item->q = chunk->q;
        //for (int dp = -1; dp <= 1; dp++) {
        //    for (int dq = -1; dq <= 1; dq++) {
        //        Chunk *other = chunk;
        //        if (dp || dq) {
        //            other = this->find_chunk(chunk->p + dp, chunk->q + dq);
        //        }
        //        if (other) {
        //            item->block_maps[dp + 1][dq + 1] = &other->map;
        //            item->light_maps[dp + 1][dq + 1] = &other->lights;
        //        }
        //        else {
        //            item->block_maps[dp + 1][dq + 1] = 0;
        //            item->light_maps[dp + 1][dq + 1] = 0;
        //        }
        //    }
        //}
        //this->compute_chunk(item);
        //this->generate_chunk(chunk, item);
        //chunk->dirty = 0;
    }

    static void map_set_func(int x, int y, int z, int w, Map *m) {
        map_set(m, x, y, z, w);
    }

    // Create a chunk that represents a unique portion of the world
    // p, q represents the chunk key
    void load_chunk(WorkerItem *item) {        
        int p = item->p;
        int q = item->q;
        
        Map *block_map = item->block_maps[1][1];
        Map *light_map = item->light_maps[1][1];
        // world.h
        static world my_world;
        auto&& gui = this->m_gui;
        my_world.create_world(p, q, cref(this->m_maze),
            map_set_func, block_map,
            gui->chunk_size, gui->show_trees, 
            gui->show_plants, gui->show_clouds);
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
        //int count = this->m_model->chunk_count;
        //State *s1 = &this->m_model->player.state;
        //for (int i = 0; i < count; i++) {
        //    Chunk *chunk = this->m_model->chunks + i;
        //    int remove_chunk = 1;
        //    int p = chunked(s1->x);
        //    int q = chunked(s1->z);
        //    if (chunk_distance(chunk, p, q) < this->m_model->delete_radius) {
        //        remove_chunk = 0;
        //    }

        //    if (remove_chunk) {
        //        map_free(&chunk->map);
        //        map_free(&chunk->lights);
        //        sign_list_free(&chunk->signs);
        //        del_buffer(chunk->buffer);
        //        del_buffer(chunk->sign_buffer);
        //        Chunk *other = this->m_model->chunks + (--count);
        //        memcpy(chunk, other, sizeof(Chunk));
        //    }
        //}
        //this->m_model->chunk_count = count;
    }

    /**
     * @brief Deletes all chunks regardless of player state
     *
     */
    void delete_all_chunks() {
        //for (int i = 0; i < this->m_model->chunk_count; i++) {
        //    Chunk *chunk = this->m_model->chunks + i;
        //    map_free(&chunk->map);
        //    map_free(&chunk->lights);
        //    sign_list_free(&chunk->signs);
        //    del_buffer(chunk->buffer);
        //    del_buffer(chunk->sign_buffer);
        //}
        //this->m_model->chunk_count = 0;
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
        //State *s = &player->state;
        //float matrix[16];
        //set_matrix_3d(matrix, this->m_model->window_width, this->m_model->window_height, s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        //float planes[6][4];
        //frustum_planes(planes, this->m_model->render_radius, matrix);
        //int p = chunked(s->x);
        //int q = chunked(s->z);
        //// int r = this->m_model->create_radius;
        //int r {this->m_model->create_radius};
        //int start = 0x0fffffff;
        //int best_score = start;
        //int best_a = 0;
        //int best_b = 0;
        //for (int dp = -r; dp <= r; dp++) {
        //    for (int dq = -r; dq <= r; dq++) {
        //        int a = p + dp;
        //        int b = q + dq;
        //        int index = (abs(a) ^ abs(b)) % NUM_WORKERS;
        //        if (index != worker->index) {
        //            continue;
        //        }
        //        Chunk *chunk = find_chunk(a, b);
        //        if (chunk && !chunk->dirty) {
        //            continue;
        //        }
        //        int distance = max(abs(dp), abs(dq));
        //        int invisible = ~chunk_visible(planes, a, b, 0, 256);
        //        int priority = 0;
        //        if (chunk) {
        //            priority = chunk->buffer & chunk->dirty;
        //        }
        //        // Check for chunk to update based on lowest score
        //        int score = (invisible << 24) | (priority << 16) | distance;
        //        if (score < best_score) {
        //            best_score = score;
        //            best_a = a;
        //            best_b = b;
        //        }
        //    }
        //}
        //if (best_score == start) {
        //    return;
        //}
        //int a = best_a;
        //int b = best_b;
        //int load = 0;
        //Chunk *chunk = find_chunk(a, b);
        //// Check if the chunk is already loaded
        //if (!chunk) {
        //    load = 1;
        //    if (this->m_model->chunk_count < MAX_CHUNKS) {
        //        chunk = this->m_model->chunks + this->m_model->chunk_count++;
        //        init_chunk(chunk, a, b);
        //    }
        //    else {
        //        return;
        //    }
        //}
        //WorkerItem *item = &worker->item;
        //item->p = chunk->p;
        //item->q = chunk->q;
        //item->load = load;
        //for (int dp = -1; dp <= 1; dp++) {
        //    for (int dq = -1; dq <= 1; dq++) {
        //        Chunk *other = chunk;
        //        if (dp || dq) {
        //            other = find_chunk(chunk->p + dp, chunk->q + dq);
        //        }
        //        if (other) {
        //            // These maps are freed using C-library free function
        //            Map *block_map = (Map*) malloc(sizeof(Map));
        //            map_copy(block_map, &other->map);
        //            Map *light_map = (Map*) malloc(sizeof(Map));
        //            map_copy(light_map, &other->lights);
        //            item->block_maps[dp + 1][dq + 1] = block_map;
        //            item->light_maps[dp + 1][dq + 1] = light_map;
        //        }
        //        else {
        //            item->block_maps[dp + 1][dq + 1] = 0;
        //            item->light_maps[dp + 1][dq + 1] = 0;
        //        }
        //    }
        //}
        //chunk->dirty = 0;
        //worker->state = WORKER_BUSY;
        //worker->cnd.notify_one();
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
            //unset_sign(x, y, z);
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

    void record_block(int x, int y, int z, int w) {
        memcpy(&this->m_model->block1, &this->m_model->block0, sizeof(Block));
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
        //State *s = &player->state;
        //this->ensure_chunks(player);
        //int p = this->chunked(s->x);
        //int q = this->chunked(s->z);
        //float light = this->get_daylight();
        //float matrix[16];
        //// matrix.cpp -> set_matrix_3d
        //set_matrix_3d(
        //    matrix, this->m_model->width, this->m_model->height,
        //    s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        //float planes[6][4];
        //// matrix.cpp -> frustum_planes
        //frustum_planes(planes, this->m_model->render_radius, matrix);
        //glUseProgram(attrib->program);
        //glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        //glUniform3f(attrib->camera, s->x, s->y, s->z);
        //glUniform1i(attrib->sampler, 0);
        //glUniform1i(attrib->extra1, 2);
        //glUniform1f(attrib->extra2, light);
        //glUniform1f(attrib->extra3, static_cast<float>(this->m_model->render_radius * this->m_gui->chunk_size));
        //glUniform1i(attrib->extra4, static_cast<int>(this->m_model->is_ortho));
        //glUniform1f(attrib->timer, this->time_of_day());
        //for (int i = 0; i < this->m_model->chunk_count; i++) {
        //    Chunk *chunk = this->m_model->chunks + i;
        //    if (this->chunk_distance(chunk, p, q) > this->m_model->render_radius) {
        //        continue;
        //    }
        //    if (!this->chunk_visible(planes, chunk->p, chunk->q, chunk->miny, chunk->maxy)) {
        //        continue;
        //    }
        //    this->draw_chunk(attrib, chunk);
        //    result += chunk->faces;
        //}
        return result;
    }

    void render_signs(Attrib *attrib, Player *player) {
        //State *s = &player->state;
        //int p = chunked(s->x);
        //int q = chunked(s->z);
        //float matrix[16];
        //set_matrix_3d(
        //    matrix, this->m_model->width, this->m_model->height,
        //    s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        //float planes[6][4];
        //frustum_planes(planes, this->m_model->render_radius, matrix);
        //glUseProgram(attrib->program);
        //glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        //glUniform1i(attrib->sampler, 3);
        //glUniform1i(attrib->extra1, 1);
        //for (int i = 0; i < this->m_model->chunk_count; i++) {
        //    Chunk *chunk = this->m_model->chunks + i;
        //    if (chunk_distance(chunk, p, q) > this->m_model->sign_radius) {
        //        continue;
        //    }
        //    if (!chunk_visible(
        //        planes, chunk->p, chunk->q, chunk->miny, chunk->maxy))
        //    {
        //        continue;
        //    }
        //    draw_signs(attrib, chunk);
        //}
    }

    void render_sign(Attrib *attrib, Player *player) {
        //if (!this->m_model->typing || this->m_model->typing_buffer[0] != CRAFT_KEY_SIGN) {
        //    return;
        //}
        //int x, y, z, face;
        //if (!hit_test_face(player, &x, &y, &z, &face)) {
        //    return;
        //}
        //State *s = &player->state;
        //float matrix[16];
        //set_matrix_3d(
        //    matrix, this->m_model->width, this->m_model->height,
        //    s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        //glUseProgram(attrib->program);
        //glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        //glUniform1i(attrib->sampler, 3);
        //glUniform1i(attrib->extra1, 1);
        //char text[MAX_SIGN_LENGTH];
        //SDL_strlcpy(text, this->m_model->typing_buffer + 1, MAX_SIGN_LENGTH);
        //text[MAX_SIGN_LENGTH - 1] = '\0';
        //float *data = malloc_faces(5, strlen(text));
        //int length = _gen_sign_buffer(data, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), face, text);
        //std::uint32_t buffer = gen_faces(5, length, data);
        //draw_sign(attrib, buffer, length);
        //del_buffer(buffer);
    }

    void render_players(Attrib *attrib, Player *player) {
        //State *s = &player->state;
        //float matrix[16];
        //set_matrix_3d(
        //    matrix, this->m_model->width, this->m_model->height,
        //    s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        //glUseProgram(attrib->program);
        //glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        //glUniform3f(attrib->camera, s->x, s->y, s->z);
        //glUniform1i(attrib->sampler, 0);
        //glUniform1f(attrib->timer, time_of_day());
        //for (int i = 0; i < this->m_model->player_count; i++) {
        //    Player *other = this->m_model->players + i;
        //    if (other != player) {
        //        draw_player(attrib, other);
        //    }
        //}
    }

    void render_sky(Attrib *attrib, Player *player, GLuint buffer) {
        State *s = &player->state;

        Matrix mvp{ MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection()) };

        glUseProgram(attrib->shader.id);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, MatrixToFloat(mvp));
        glUniform1i(attrib->sampler, 2);
        glUniform1f(attrib->timer, time_of_day());

        draw_triangles_3d(attrib, buffer, 512 * 3);
    }

    void render_wireframe(Attrib *attrib, Player *player) {
        //State *s = &player->state;
        //float matrix[16];
        //set_matrix_3d(
        //    matrix, this->m_model->width, this->m_model->height,
        //    s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        //int hx, hy, hz;
        //int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        //if (is_obstacle(hw)) {
        //    glUseProgram(attrib->program);
        //    glLineWidth(1);
        //    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        //    std::uint32_t wireframe_buffer = gen_wireframe_buffer(static_cast<float>(hx), static_cast<float>(hy), static_cast<float>(hz), 0.53f);
        //    draw_lines(attrib, wireframe_buffer, 3, 24);
        //    del_buffer(wireframe_buffer);
        //}
    }

    void render_crosshairs(Attrib *attrib) {
        //float matrix[16];
        //set_matrix_2d(matrix, this->m_model->width, this->m_model->height);
        //glUseProgram(attrib->program);
        //glLineWidth(static_cast<float>(4 * this->m_model->scale));
        //glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        //std::uint32_t crosshair_buffer = gen_crosshair_buffer();
        //draw_lines(attrib, crosshair_buffer, 2, 4);
        //del_buffer(crosshair_buffer);
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

    void parse_command(const char *buffer) {
        int radius, count, xc, yc, zc;
        if (SDL_sscanf(buffer, "/view %d", &radius) == 1) {
            if (radius >= 1 && radius <= 24) {
                this->m_model->create_radius = radius;
                this->m_model->render_radius = radius;
                this->m_model->delete_radius = radius + 4;
            }
            else {
                // Notify user with view correct parameters
            }
        }
    } // parse command

    void on_light() {
        State *s = &this->m_model->player.state;
        int hx, hy, hz;
        int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        if (hy > 0 && hy < 256 && is_destructable(hw)) {
            toggle_light(hx, hy, hz);
        }
    }

    void on_left_click() {
        State *s = &this->m_model->player.state;
        int hx, hy, hz;
        int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        if (hy > 0 && hy < 256 && is_destructable(hw)) {
            set_block(hx, hy, hz, 0);
            record_block(hx, hy, hz, 0);
#if defined(MAZE_DEBUG)
            printf("INFO: on_left_click(x: %d, y: %d, z: %d, w: %d, Block Id: %d): \n", hx, hy, hz, hw, items[this->m_model->item_index]);
#endif
            if (is_plant(get_block(hx, hy + 1, hz))) {
                set_block(hx, hy + 1, hz, 0);
            }
        }
    }

    void on_right_click() {
        State *s = &this->m_model->player.state;
        int hx, hy, hz;
        int hw = hit_test(1, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        if (hy > 0 && hy < 256 && is_obstacle(hw)) {
            if (!player_intersects_block(2, s->x, s->y, s->z, hx, hy, hz)) {
                set_block(hx, hy, hz, items[this->m_model->item_index]);
                record_block(hx, hy, hz, items[this->m_model->item_index]);
#if defined(MAZE_DEBUG)
                printf("INFO: on_right_click(%d, %d, %d, %d, Block Id: %d): \n", hx, hy, hz, hw, items[this->m_model->item_index]);
#endif
            }
        }
    }

    void on_middle_click() {
        State* s = &this->m_model->player.state;
        int hx, hy, hz;
        int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        for (int i = 0; i < item_count; i++) {
            if (items[i] == hw) {
                this->m_model->item_index = i;
                break;
            }
        }
    }

    ///**
    //* Reference: https://github.com/rswinkle/Craft/blob/sdl/src/main.c
    //* @brief Handle SDL events
    //* @param dt
    //* @param running is a reference to game loop invariant
    //* @return bool return true when events are handled successfully
    //*/
    //bool handle_events(double dt, bool& running, const function<mazes::maze_types(const string& algo)>& get_maze_algo_from_str, const function<int(int, int)>& get_int, const mt19937& rng) {
    //    static float dy = 0;
    //    State* s = &this->m_model->players->state;
    //    int sz = 0;
    //    int sx = 0;
    //    float mouse_mv = 0.0025f;
    //    float dir_mv = 0.025f;
    //    int sc = -1, code = -1;

    //    SDL_Keymod mod_state = SDL_GetModState();

    //    bool imgui_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);

    //    int control = mod_state;
    //    SDL_Event e;
    //    while (SDL_PollEvent(&e)) {
    //        ImGui_ImplSDL3_ProcessEvent(&e);

    //        switch (e.type) {
    //        case SDL_EVENT_QUIT: {
    //            running = false;
    //            break;
    //        }
    //        case SDL_EVENT_KEY_UP: {
    //            sc = e.key.scancode;
    //            switch (sc) {
    //            }
    //            break;
    //        }
    //        case SDL_EVENT_KEY_DOWN: {

    //            sc = e.key.scancode;
    //            switch (sc) {
    //            case SDL_SCANCODE_ESCAPE: {
    //                SDL_SetWindowRelativeMouseMode(this->m_model->window, SDL_FALSE);
    //                this->m_gui->capture_mouse = false;
    //                this->m_gui->fullscreen = false;
    //                this->m_model->typing = 0;
    //                break;
    //            }
    //            case SDL_SCANCODE_RETURN: {
    //                if (!imgui_focused && this->m_model->typing) {
    //                    if (mod_state) {
    //                        if (this->m_model->text_len < MAX_TEXT_LENGTH - 1) {
    //                            this->m_model->typing_buffer[this->m_model->text_len] = '\n';
    //                            this->m_model->typing_buffer[this->m_model->text_len + 1] = '\0';
    //                        }
    //                    } else {
    //                        this->m_model->typing = 0;
    //                        if (this->m_model->typing_buffer[0] == CRAFT_KEY_SIGN) {
    //                            Player* player = this->m_model->players;
    //                            int x, y, z, face;
    //                            if (hit_test_face(player, &x, &y, &z, &face)) {
    //                                set_sign(x, y, z, face, this->m_model->typing_buffer + 1);
    //                            }
    //                        } else if (this->m_model->typing_buffer[0] == '/') {
    //                            this->parse_command(this->m_model->typing_buffer, cref(get_maze_algo_from_str), cref(get_int), cref(rng));
    //                        }
    //                    }
    //                } else {
    //                    if (control) {
    //                        this->on_right_click();
    //                    } else {
    //                        this->on_left_click();
    //                    }
    //                }
    //                break;
    //            }
    //            case SDL_SCANCODE_V: {
    //                if (control) {
    //                    auto clip_buffer = const_cast<char*>(SDL_GetClipboardText());
    //                    if (this->m_model->typing) {
    //                        this->m_model->suppress_char = 1;
    //                        SDL_strlcat(this->m_model->typing_buffer, clip_buffer,
    //                            MAX_TEXT_LENGTH - this->m_model->text_len - 1);
    //                    } else {
    //                        parse_command(clip_buffer, cref(get_maze_algo_from_str), cref(get_int), cref(rng));
    //                    }
    //                    free(clip_buffer);
    //                }
    //                break;
    //            }
    //            case SDL_SCANCODE_0:
    //            case SDL_SCANCODE_1:
    //            case SDL_SCANCODE_2:
    //            case SDL_SCANCODE_3:
    //            case SDL_SCANCODE_4:
    //            case SDL_SCANCODE_5:
    //            case SDL_SCANCODE_6:
    //            case SDL_SCANCODE_7:
    //            case SDL_SCANCODE_8:
    //            case SDL_SCANCODE_9: {
    //                if (!imgui_focused && !this->m_model->typing)
    //                    this->m_model->item_index = (sc - SDL_SCANCODE_1);
    //                break;
    //            }
    //            case KEY_FLY: {
    //                if (!imgui_focused && !this->m_model->typing)
    //                    this->m_model->flying = ~this->m_model->flying;
    //                break;
    //            }
    //            case KEY_ITEM_NEXT: {
    //                if (!imgui_focused && !this->m_model->typing)
    //                    this->m_model->item_index = (this->m_model->item_index + 1) % item_count;
    //                break;
    //            }
    //            case KEY_ITEM_PREV: {
    //                if (!imgui_focused && !this->m_model->typing) {
    //                    this->m_model->item_index--;
    //                    if (this->m_model->item_index < 0)
    //                        this->m_model->item_index = item_count - 1;
    //                }
    //                break;
    //            }
    //            case KEY_OBSERVE: {
    //                if (!imgui_focused && !this->m_model->typing)
    //                    this->m_model->observe1 = (this->m_model->observe1 + 1) % this->m_model->player_count;
    //                break;
    //            }
    //            case KEY_OBSERVE_INSET: {
    //                if (!imgui_focused && !this->m_model->typing)
    //                    this->m_model->observe2 = (this->m_model->observe2 + 1) % this->m_model->player_count;
    //                break;
    //            }
    //            case KEY_CHAT: {
    //                if (!imgui_focused && !this->m_model->typing) {
    //                    this->m_model->typing = 1;
    //                    this->m_model->typing_buffer[0] = '\0';
    //                    this->m_model->text_len = 0;
    //                    SDL_StartTextInput(this->m_model->window);
    //                }
    //                break;
    //            }
    //            case KEY_COMMAND: {
    //                if (!imgui_focused && !this->m_model->typing) {
    //                    this->m_model->typing = 1;
    //                    this->m_model->typing_buffer[0] = '\0';
    //                    SDL_StartTextInput(this->m_model->window);
    //                }
    //                break;
    //            }
    //            case KEY_SIGN: {
    //                if (!imgui_focused && this->m_gui->capture_mouse) {
    //                    this->m_model->typing = 1;
    //                    this->m_model->typing_buffer[0] = '\0';
    //                    SDL_StartTextInput(this->m_model->window);
    //                }
    //                break;

    //            }
    //            } // switch
    //            break;
    //        } // case SDL_EVENT_KEY_DOWN
    //        case SDL_EVENT_TEXT_INPUT: {
    //            if (!imgui_focused && this->m_model->typing && this->m_model->text_len < MAX_TEXT_LENGTH - 1) {
    //                SDL_strlcat(this->m_model->typing_buffer, e.text.text, this->m_model->text_len);
    //                this->m_model->text_len += strlen(e.text.text);
    //            }
    //            break;
    //        }
    //        case SDL_EVENT_MOUSE_MOTION: {
    //            if (this->m_gui->capture_mouse && SDL_GetWindowRelativeMouseMode(this->m_model->window)) {
    //                s->rx += e.motion.xrel * mouse_mv;
    //                if (INVERT_MOUSE) {
    //                    s->ry += e.motion.yrel * mouse_mv;
    //                } else {
    //                    s->ry -= e.motion.yrel * mouse_mv;
    //                }
    //                if (s->rx < 0) {
    //                    s->rx += RADIANS(360);
    //                }
    //                if (s->rx >= RADIANS(360)) {
    //                    s->rx -= RADIANS(360);
    //                }
    //                s->ry = max(s->ry, -RADIANS(90));
    //                s->ry = min(s->ry, RADIANS(90));
    //            }
    //            break;
    //        }
    //        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
    //            if (!imgui_focused && SDL_GetWindowRelativeMouseMode(this->m_model->window) && e.button.button == SDL_BUTTON_LEFT) {
    //                if (control) {
    //                    on_right_click();
    //                } else {
    //                    on_left_click();
    //                }
    //            } else if (!imgui_focused && SDL_GetWindowRelativeMouseMode(this->m_model->window) && e.button.button == SDL_BUTTON_RIGHT) {
    //                if (control) {
    //                    on_light();
    //                } else {
    //                    on_right_click();
    //                }
    //            } else if (e.button.button == SDL_BUTTON_MIDDLE) {
    //                if (!imgui_focused && SDL_GetWindowRelativeMouseMode(this->m_model->window)) {
    //                    on_middle_click();
    //                }
    //            }

    //            break;
    //        }
    //        case SDL_EVENT_MOUSE_WHEEL: {
    //            if (!imgui_focused && SDL_GetWindowRelativeMouseMode(this->m_model->window)) {
    //                // TODO might have to change this to force 1 step
    //                if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL) {
    //                    this->m_model->item_index += e.wheel.y;
    //                } else {
    //                    this->m_model->item_index -= e.wheel.y;
    //                }
    //                if (this->m_model->item_index < 0)
    //                    this->m_model->item_index = item_count - 1;
    //                else
    //                    this->m_model->item_index %= item_count;
    //            }
    //            break;
    //        }
    //        case SDL_EVENT_WINDOW_RESIZED: {
    //            this->m_model->scale = get_scale_factor();
    //            SDL_GetWindowSizeInPixels(this->m_model->window, &this->m_model->width, &this->m_model->height);
    //            break;
    //        }
    //        case SDL_EVENT_WINDOW_SHOWN: {
    //            this->m_model->scale = get_scale_factor();
    //            SDL_GetWindowSizeInPixels(this->m_model->window, &this->m_model->width, &this->m_model->height);
    //            break;
    //        }
    //        } // switch
    //    } // SDL_Event

    //    // Close the app, events handled successfully
    //    if (!running) {
    //        return true;
    //    }

    //    const Uint8* state = SDL_GetKeyboardState(nullptr);

    //    if (!(imgui_focused || this->m_model->typing)) {
    //        this->m_model->is_ortho = state[KEY_ORTHO] ? 64 : 0;
    //        this->m_model->fov = state[KEY_ZOOM] ? 15 : 65;
    //        if (state[KEY_FORWARD]) sz--;
    //        if (state[KEY_BACKWARD]) sz++;
    //        if (state[KEY_LEFT]) sx--;
    //        if (state[KEY_RIGHT]) sx++;
    //        if (state[SDL_SCANCODE_LEFT]) s->rx -= dir_mv;
    //        if (state[SDL_SCANCODE_RIGHT]) s->rx += dir_mv;
    //        if (state[SDL_SCANCODE_UP]) s->ry += dir_mv;
    //        if (state[SDL_SCANCODE_DOWN]) s->ry -= dir_mv;
    //    }

    //    float vx, vy, vz;
    //    get_motion_vector(this->m_model->flying, sz, sx, s->rx, s->ry, &vx, &vy, &vz);
    //    if (!this->m_model->typing) {
    //        if (state[KEY_JUMP] && this->m_gui->capture_mouse) {
    //            if (this->m_model->flying) {
    //                vy = 1;
    //            } else if (dy == 0) {
    //                dy = 8;
    //            }
    //        }
    //    }
    //    float speed = this->m_model->flying ? 20 : 5;
    //    int estimate = roundf(sqrtf(
    //        powf(vx * speed, 2) +
    //        powf(vy * speed + SDL_abs(dy) * 2, 2) +
    //        powf(vz * speed, 2)) * dt * 8);
    //    int step = max(8, estimate);
    //    float ut = dt / step;
    //    vx = vx * ut * speed;
    //    vy = vy * ut * speed;
    //    vz = vz * ut * speed;
    //    for (int i = 0; i < step; i++) {
    //        if (this->m_model->flying) {
    //            dy = 0;
    //        } else {
    //            dy -= ut * 25;
    //            dy = max(dy, -250);
    //        }
    //        s->x += vx;
    //        s->y += vy + dy * ut;
    //        s->z += vz;
    //        if (collide(2, &s->x, &s->y, &s->z)) {
    //            dy = 0;
    //        }
    //    }
    //    if (s->y < 0) {
    //        s->y = highest_block(s->x, s->z) + 2;
    //    }

        return true;
    } // handle_events

    void reset_model() {
        memset(this->m_model->chunks, 0, sizeof(Chunk) * MAX_CHUNKS);
        this->m_model->chunk_count = 0;
        memset(&this->m_model->player, 0, sizeof(Player));
        this->m_model->flying = false;
        this->m_model->item_index = 0;
        this->m_model->day_length = DAY_LENGTH;
        this->m_model->start_time = (this->m_model->day_length / 3) * 1000;
        this->m_model->time_changed = true;
        this->m_model->scale = 1;
        this->m_model->create_radius = CREATE_CHUNK_RADIUS;
        this->m_model->render_radius = RENDER_CHUNK_RADIUS;
        this->m_model->delete_radius = DELETE_CHUNK_RADIUS;
        this->m_model->sign_radius = RENDER_SIGN_RADIUS;
    }

}; // craft_impl

craft::craft(const std::string& window_name, const std::string& version, const std::string& help)
    : m_pimpl{std::make_unique<craft_impl>(cref(window_name), cref(version), cref(help))} {
}

craft::~craft() = default;


/**
 * Run the craft-engine in a loop with SDL window open, compute the maze first
*/
bool craft::run(unsigned long seed, const std::list<std::string>& algos, 
    const std::function<mazes::maze_types(const std::string& algo)>& get_maze_algo_from_str,
    const std::function<int(int, int)>& get_int, std::mt19937& rng) const noexcept {

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, m_pimpl->m_window_name.c_str());
    SetWindowIcon(LoadImage("textures/mb_window_icon.png"));

#if defined(MAZE_DEBUG)
    std::cout << "INFO: Launching Craft rendering engine. . ." << std::endl;
#endif

    // Define the camera to look into our 3d world
    Camera camera = { 0 };
    camera.position = { 0.2f, 0.4f, 0.2f };
    camera.target = { 0.185f, 0.4f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    DisableCursor();

    this->m_pimpl->m_model->start_time = static_cast<int>(GetFrameTime());

    SetTargetFPS(60);

    // Load images to RAM
    Image img1 = LoadImage("textures/texture.png");
    Image img2 = LoadImage("textures/font.png");
    Image img3 = LoadImage("textures/sky.png");
    Image img4 = LoadImage("textures/sign.png");
    // Load textures to VRAM
    Texture2D texture1 = LoadTextureFromImage(img1);
    Texture2D texture2 = LoadTextureFromImage(img2);
    Texture2D texture3 = LoadTextureFromImage(img3);
    Texture2D texture4 = LoadTextureFromImage(img4);
    UnloadImage(img1);
    UnloadImage(img2);
    UnloadImage(img3);
    UnloadImage(img4);

    // LOAD SHADERS 
    craft_impl::Attrib block_attrib;
    craft_impl::Attrib line_attrib;
    craft_impl::Attrib text_attrib;
    craft_impl::Attrib sky_attrib;

#if defined(__EMSCRIPTEN__)
    //program = load_program("shaders/es/block_vertex.es.glsl", "shaders/es/block_fragment.es.glsl");
#else
    Shader block_shader = LoadShader("shaders/block_vertex.glsl", "shaders/block_fragment.glsl");
    block_attrib.shader = block_shader;
#endif
    block_attrib.position = 0;
    block_attrib.normal = 1;
    block_attrib.uv = 2;
    block_attrib.matrix = GetShaderLocation(block_attrib.shader, "matrix");
    block_attrib.sampler = GetShaderLocation(block_attrib.shader, "sampler");
    block_attrib.extra1 = GetShaderLocation(block_attrib.shader, "sky_sampler");
    block_attrib.extra2 = GetShaderLocation(block_attrib.shader, "daylight");
    block_attrib.extra3 = GetShaderLocation(block_attrib.shader, "fog_distance");
    block_attrib.extra4 = GetShaderLocation(block_attrib.shader, "is_ortho");
    block_attrib.camera = GetShaderLocation(block_attrib.shader, "camera");
    block_attrib.timer = GetShaderLocation(block_attrib.shader, "timer");

#if defined(__EMSCRIPTEN__)
    //program = load_program("shaders/es/line_vertex.es.glsl", "shaders/es/line_fragment.es.glsl");
#else
    Shader line_shader = LoadShader("shaders/line_vertex.glsl", "shaders/line_fragment.glsl");
    line_attrib.shader = line_shader;
#endif
    line_attrib.position = 0;
    line_attrib.matrix = GetShaderLocation(line_attrib.shader, "matrix");

#if defined(__EMSCRIPTEN__)
    //program = load_program("shaders/es/text_vertex.es.glsl", "shaders/es/text_fragment.es.glsl");
#else
    Shader text_shader = LoadShader("shaders/text_vertex.glsl", "shaders/text_fragment.glsl");
    text_attrib.shader = text_shader;
#endif
    text_attrib.position = 0;
    text_attrib.uv = 1;
    text_attrib.matrix = GetShaderLocation(text_attrib.shader, "matrix");
    text_attrib.sampler = GetShaderLocation(text_attrib.shader, "sampler");
    text_attrib.extra1 = GetShaderLocation(text_attrib.shader, "is_sign");

#if defined(__EMSCRIPTEN__)
    //program = load_program("shaders/es/sky_vertex.es.glsl", "shaders/es/sky_fragment.es.glsl");
#else
    Shader sky_shader = LoadShader("shaders/sky_vertex.glsl", "shaders/sky_fragment.glsl");
    sky_attrib.shader = sky_shader;
#endif
    sky_attrib.position = 0;
    sky_attrib.normal = 1;
    sky_attrib.uv = 2;
    sky_attrib.matrix = GetShaderLocation(sky_attrib.shader, "matrix");
    sky_attrib.sampler = GetShaderLocation(sky_attrib.shader, "sampler");
    sky_attrib.timer = GetShaderLocation(sky_attrib.shader, "timer");

    // INITIALIZE WORKER THREADS
    //m_pimpl->init_worker_threads();

    // DATABASE INITIALIZATION 
    if (USE_CACHE) {
        db_enable();
        static char *db_path = "craft.db";
        if (db_init(db_path)) {
            cerr << "ERROR: db init failed!!" << std::endl;
            return false;
        }
#if defined(MAZE_DEBUG)
        std::cout << "INFO: Writing to db file: " << db_path << std::endl;
#endif
    }

    // LOCAL VARIABLES 
    uint64_t last_commit = static_cast<uint64_t>(GetFrameTime());

    //uint32_t sky_buffer = gen_sky_buffer();

    craft_impl::Player *me = &m_pimpl->m_model->player;
    craft_impl::State *p_state = &m_pimpl->m_model->player.state;
    me->id = 0;
    me->name = "me";
    me->buffer = 0;

    // LOAD STATE FROM DATABASE 
    int loaded = db_load_state(&p_state->x, &p_state->y, &p_state->z, &p_state->rx, &p_state->ry);
    if (loaded)
        p_state->y = 75.f;

    // Init some local vars for handling maze duties
    auto my_maze_type = get_maze_algo_from_str(algos.back());
    auto&& gui = this->m_pimpl->m_gui;
    auto&& maze2 = this->m_pimpl->m_maze;

    auto make_maze_ptr = [this, &my_maze_type, &get_int, &rng, &maze2](unsigned int w, unsigned int l, unsigned int h) {
        maze2 = std::make_unique<maze_thread_safe>(my_maze_type, get_int, rng, w, l, h, items[this->m_pimpl->m_model->item_index]);
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


    int triangle_faces = 0;

    //m_pimpl->force_chunks(me);
    bool running = true;

    rlViewport(0, 0, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT);

    Mesh my_cube = GenMeshCube(10, 10, 10);
    Material my_material = LoadMaterialDefault();

    // BEGIN EVENT LOOP
#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (running && !WindowShouldClose())
#endif
    {
        // Updates
        // FLUSH DATABASE 
        auto elapsed = static_cast<uint32_t>(GetFrameTime());
        if (elapsed > COMMIT_INTERVAL) {
            db_commit();
        }
        UpdateCamera(&camera, CAMERA_FIRST_PERSON);

        static bool capture_mouse = false;
        if (IsKeyPressed(KEY_ESCAPE)) {
			capture_mouse = !capture_mouse;
			if (capture_mouse) {
				DisableCursor();
			} else {
				EnableCursor();
			}
		}

        // Handle Maze events
        // Check if maze is available and then perform two async operations:
        // 1. Set maze string and compute maze geometry for 3D coordinates (includes a height value)
        // 2. Write the maze to a Wavefront object file using the computed data (except the default maze)
//        if (maze_gen_future.valid() && maze_gen_future.wait_for(chrono::seconds(0)) == future_status::ready) {
//            // Reset player state to roughly the origin
//            p_state->y = 75.f;
//            p_state->x = 100.f;
//            p_state->z = 100.f;
//            // Get the maze and reset the future
//            maze_gen_future.get();
//            // Don't write the first maze that loads when app starts
//        }
//
//        if (0) {
//            // Writing the maze will run in the background - only do that on Desktop
//            write_maze_now = false;
//            // Only write the maze when **NOT** on the web browser
//#if !defined(__EMSCRIPTEN__ )
//            write_success = maze_writer_fut(gui->outfile);
//#endif
//            this->m_pimpl->m_gui->maze_json = json_writer(gui->outfile);
//        } else {
//            // Failed to set maze
//        }
//
    
        
    
        /*m_pimpl->delete_chunks();
        m_pimpl->del_buffer(me->buffer);

        me->buffer = m_pimpl->gen_player_buffer(p_state->x, p_state->y, p_state->z, p_state->rx, p_state->ry);
        m_pimpl->interpolate_player(me);*/

        // RENDER 3-D SCENE
        BeginDrawing();
        ClearBackground(RAYWHITE);

#if defined(MAZE_DEBUG)
        DrawMesh(my_cube, my_material, MatrixTranslate(0, 5, -10));
#endif

        //m_pimpl->render_sky(&sky_attrib, me, sky_buffer);

        //glClear(GL_DEPTH_BUFFER_BIT);
        rlClearScreenBuffers();

        //triangle_faces = m_pimpl->render_chunks(&block_attrib, me);
        //m_pimpl->render_signs(&text_attrib, me);
        //m_pimpl->render_sign(&text_attrib, me);
        //m_pimpl->render_players(&block_attrib, me);
        //if (gui->show_wireframes) {
        //    m_pimpl->render_wireframe(&line_attrib, me);
        //}

        // RENDER HUD 
        //glClear(GL_DEPTH_BUFFER_BIT);
        //if (gui->show_crosshairs) {
        //    m_pimpl->render_crosshairs(&line_attrib);
        //}
        //if (gui->show_items) {
        //    m_pimpl->render_item(&block_attrib);
        //}

        DrawFPS(10, 10);
        EndDrawing();

        if (!running) {
#if defined(__EMSCRIPTEN__)
            emscripten_cancel_main_loop();
#endif
        }
    } // EVENT LOOP

#if defined(__EMSCRIPTEN__)
        EMSCRIPTEN_MAINLOOP_END;
#endif

#if defined(MAZE_DEBUG)
    std::cout << "INFO: Cleaning up worker threads. . ." << std::endl;
#endif

    //m_pimpl->cleanup_worker_threads();

#if defined(MAZE_DEBUG)
    std::cout << "INFO: Closing DB. . ." << std::endl;
#endif

    db_save_state(p_state->x, p_state->y, p_state->z, p_state->rx, p_state->ry);
    db_close();
    db_disable();

#if defined(MAZE_DEBUG)
    std::cout << "INFO: Deleting buffer objects. . ." << std::endl;
#endif

    //m_pimpl->delete_all_chunks();
    UnloadShader(block_attrib.shader);
    UnloadShader(text_attrib.shader);
    UnloadShader(sky_attrib.shader);
    UnloadShader(line_attrib.shader);
    UnloadTexture(texture1);
    UnloadTexture(texture2);
    UnloadTexture(texture3);
    UnloadTexture(texture4);

    CloseWindow();

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
