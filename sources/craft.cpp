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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
        Player players[1];
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

    //void draw_triangles_3d_ao(Attrib *attrib, std::uint32_t buffer, int count) {
    //    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    //    glEnableVertexAttribArray(attrib->position);
    //    glEnableVertexAttribArray(attrib->normal);
    //    glEnableVertexAttribArray(attrib->uv);
    //    glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
    //        sizeof(float) * 10, 0);
    //    glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE,
    //        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 3));
    //    glVertexAttribPointer(attrib->uv, 4, GL_FLOAT, GL_FALSE,
    //        sizeof(float) * 10, (GLvoid *)(sizeof(float) * 6));
    //    glDrawArrays(GL_TRIANGLES, 0, count);
    //    glDisableVertexAttribArray(attrib->position);
    //    glDisableVertexAttribArray(attrib->normal);
    //    glDisableVertexAttribArray(attrib->uv);
    //    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //}

    //void draw_triangles_3d_text(Attrib *attrib, std::uint32_t buffer, int count) {
    //    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    //    glEnableVertexAttribArray(attrib->position);
    //    glEnableVertexAttribArray(attrib->uv);
    //    glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
    //        sizeof(float) * 5, 0);
    //    glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
    //        sizeof(float) * 5, (GLvoid *)(sizeof(float) * 3));
    //    glDrawArrays(GL_TRIANGLES, 0, count);
    //    glDisableVertexAttribArray(attrib->position);
    //    glDisableVertexAttribArray(attrib->uv);
    //    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //}

    //void draw_triangles_3d(Attrib *attrib, GLuint buffer, int count) {
    //    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    //    glEnableVertexAttribArray(attrib->position);

    //    glEnableVertexAttribArray(attrib->normal);
    //    glEnableVertexAttribArray(attrib->uv);

    //    glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, 0);
    //    glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid *)(sizeof(float) * 3));
    //    glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid *)(sizeof(float) * 6));
    //    
    //    glDrawArrays(GL_TRIANGLES, 0, count);

    //    glDisableVertexAttribArray(attrib->position);
    //    glDisableVertexAttribArray(attrib->normal);
    //    glDisableVertexAttribArray(attrib->uv);
    //    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //}

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

    void update_player(Player *player, float x, float y, float z, float rx, float ry, int interpolate) {
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

    //int highest_block(float x, float z) const {
    //    int result = -1;
    //    int nx = static_cast<int>(roundf(x));
    //    int nz = static_cast<int>(roundf(z));
    //    int p = this->chunked(x);
    //    int q = this->chunked(z);
    //    Chunk *chunk = this->find_chunk(p, q);
    //    if (chunk) {
    //        Map *map = &chunk->map;
    //        MAP_FOR_EACH(map, ex, ey, ez, ew) {
    //            // item.h -> is_obstacle
    //            if (is_obstacle(ew) && ex == nx && ez == nz) {
    //                result = max(result, ey);
    //            }
    //        } END_MAP_FOR_EACH;
    //    }
    //    return result;
    //}


    void dirty_chunk(Chunk *chunk) const {
        //chunk->dirty = 1;
        //if (has_lights(chunk)) {
        //    for (int dp = -1; dp <= 1; dp++) {
        //        for (int dq = -1; dq <= 1; dq++) {
        //            Chunk *other = this->find_chunk(chunk->p + dp, chunk->q + dq);
        //            if (other) {
        //                other->dirty = 1;
        //            }
        //        }
        //    }
        //}
    }

    // Handles terrain generation in a multithreaded environment
    void compute_chunk(WorkerItem *item) const {
        //char *opaque = (char *)calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
        //char *light = (char *)calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
        //char *highest = (char *)calloc(XZ_SIZE * XZ_SIZE, sizeof(char));

        //int ox = item->p * this->m_gui->chunk_size - this->m_gui->chunk_size - 1;
        //int oy = -1;
        //int oz = item->q * this->m_gui->chunk_size - this->m_gui->chunk_size - 1;

        //// check for lights
        //int has_light = 0;
        //if (this->m_gui->show_lights) {
        //    for (int a = 0; a < 3; a++) {
        //        for (int b = 0; b < 3; b++) {
        //            Map *map = item->light_maps[a][b];
        //            if (map && map->size) {
        //                has_light = 1;
        //            }
        //        }
        //    }
        //}

        //// populate opaque array
        //for (int a = 0; a < 3; a++) {
        //    for (int b = 0; b < 3; b++) {
        //        Map *block_map = item->block_maps[a][b];
        //        if (!block_map) {
        //            continue;
        //        }
        //        MAP_FOR_EACH(block_map, ex, ey, ez, ew) {
        //            int x = ex - ox;
        //            int y = ey - oy;
        //            int z = ez - oz;
        //            int w = ew;
        //            // TODO: this should be unnecessary
        //            if (x < 0 || y < 0 || z < 0) {
        //                continue;
        //            }
        //            if (x >= XZ_SIZE || y >= Y_SIZE || z >= XZ_SIZE) {
        //                continue;
        //            }
        //            // END TODO
        //            opaque[XYZ(x, y, z)] = !is_transparent(w);
        //            if (opaque[XYZ(x, y, z)]) {
        //                highest[XZ(x, z)] = max(static_cast<int>(highest[XZ(x, z)]), y);
        //            }
        //        } END_MAP_FOR_EACH;
        //    }
        //}

        //// flood fill light intensities
        //if (has_light) {
        //    for (int a = 0; a < 3; a++) {
        //        for (int b = 0; b < 3; b++) {
        //            Map *map = item->light_maps[a][b];
        //            if (!map) {
        //                continue;
        //            }
        //            MAP_FOR_EACH(map, ex, ey, ez, ew) {
        //                int x = ex - ox;
        //                int y = ey - oy;
        //                int z = ez - oz;
        //                light_fill(opaque, light, x, y, z, ew, 1);
        //            } END_MAP_FOR_EACH;
        //        }
        //    }
        //}

        //Map *block_map = item->block_maps[1][1];

        //// count exposed faces
        //int miny = 256;
        //int maxy = 0;
        //int faces = 0;
        //MAP_FOR_EACH(block_map, ex, ey, ez, ew) {
        //    if (ew <= 0) {
        //        continue;
        //    }
        //    int x = ex - ox;
        //    int y = ey - oy;
        //    int z = ez - oz;
        //    int f1 = !opaque[XYZ(x - 1, y, z)];
        //    int f2 = !opaque[XYZ(x + 1, y, z)];
        //    int f3 = !opaque[XYZ(x, y + 1, z)];
        //    int f4 = !opaque[XYZ(x, y - 1, z)] && (ey > 0);
        //    int f5 = !opaque[XYZ(x, y, z - 1)];
        //    int f6 = !opaque[XYZ(x, y, z + 1)];
        //    int total = f1 + f2 + f3 + f4 + f5 + f6;
        //    if (total == 0) {
        //        continue;
        //    }
        //    if (is_plant(ew)) {
        //        total = 4;
        //    }
        //    miny = min(miny, ey);
        //    maxy = max(maxy, ey);
        //    faces += total;
        //} END_MAP_FOR_EACH;

        //// generate geometry
        //// each vertex has 10 components (x, y, z, nx, ny, nz, u, v, ao, light)
        //static constexpr int components = 10;
        //float *data = malloc_faces(components, faces);
        //int offset = 0;
        //MAP_FOR_EACH(block_map, ex, ey, ez, ew) {
        //    if (ew <= 0) {
        //        continue;
        //    }
        //    int x = ex - ox;
        //    int y = ey - oy;
        //    int z = ez - oz;
        //    int f1 = !opaque[XYZ(x - 1, y, z)];
        //    int f2 = !opaque[XYZ(x + 1, y, z)];
        //    int f3 = !opaque[XYZ(x, y + 1, z)];
        //    int f4 = !opaque[XYZ(x, y - 1, z)] && (ey > 0);
        //    int f5 = !opaque[XYZ(x, y, z - 1)];
        //    int f6 = !opaque[XYZ(x, y, z + 1)];
        //    int total = f1 + f2 + f3 + f4 + f5 + f6;
        //    if (total == 0) {
        //        continue;
        //    }
        //    char neighbors[27] = {0};
        //    char lights[27] = {0};
        //    float shades[27] = {0};
        //    int index = 0;
        //    for (int dx = -1; dx <= 1; dx++) {
        //        for (int dy = -1; dy <= 1; dy++) {
        //            for (int dz = -1; dz <= 1; dz++) {
        //                neighbors[index] = opaque[XYZ(x + dx, y + dy, z + dz)];
        //                lights[index] = light[XYZ(x + dx, y + dy, z + dz)];
        //                shades[index] = 0;
        //                if (y + dy <= highest[XZ(x + dx, z + dz)]) {
        //                    for (int oy = 0; oy < 8; oy++) {
        //                        if (opaque[XYZ(x + dx, y + dy + oy, z + dz)]) {
        //                            shades[index] = 1.0f - oy * 0.125f;
        //                            break;
        //                        }
        //                    }
        //                }
        //                index++;
        //            }
        //        }
        //    }
        //    float ao[6][4];
        //    float light[6][4];
        //    occlusion(neighbors, lights, shades, ao, light);
        //    if (is_plant(ew)) {
        //        total = 4;
        //        float min_ao = 1;
        //        float max_light = 0;
        //        for (int a = 0; a < 6; a++) {
        //            for (int b = 0; b < 4; b++) {
        //                min_ao = min(min_ao, ao[a][b]);
        //                max_light = max(max_light, light[a][b]);
        //            }
        //        }
        //        float rotation = simplex2(static_cast<float>(ex), static_cast<float>(ez), 4, 0.5f, 2.f) * 360.f;
        //        /*make_plant(
        //            data + offset, min_ao, max_light,
        //            static_cast<float>(ex), static_cast<float>(ey), static_cast<float>(ez),
        //            0.5f, ew, rotation);*/
        //    }
        //    else {
        //        //make_cube(
        //        //    data + offset, ao, light,
        //        //    f1, f2, f3, f4, f5, f6,
        //        //    static_cast<float>(ex), static_cast<float>(ey), static_cast<float>(ez), 0.5f, ew);
        //    }
        //    offset += total * 60;
        //} END_MAP_FOR_EACH;

        //free(opaque);
        //free(light);
        //free(highest);

        //item->miny = miny;
        //item->maxy = maxy;
        //item->faces = faces;
        //item->data = data;
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


    void reset_model() {

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

    craft_impl::Player *me = &m_pimpl->m_model->players[0];
    craft_impl::State *p_state = &m_pimpl->m_model->players[0].state;
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
        maze2 = std::make_unique<maze_thread_safe>(my_maze_type, get_int, rng, w, l, h, 1);
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

    Mesh my_cube = GenMeshCube(100, 100, 100);
    Material my_material = LoadMaterialDefault();
    Model my_model = LoadModelFromMesh(my_cube);

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

        DrawModel(my_model, { 0.0f, 0.0f, 0.0f }, 1.0f, DARKBLUE);

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
