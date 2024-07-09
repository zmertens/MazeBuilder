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
#include <unordered_map>
#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <random>
#include <utility>

#include <noise/noise.h>
#include <tinycthread/tinycthread.h>

#include "util.h"
#include "world.h"
#include "cube.h"
#include "db.h"
#include "item.h"
#include "matrix.h"

#include "maze_types_enum.h"
#include "maze_factory.h"
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

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 768
#define VSYNC 1
#define SCROLL_THRESHOLD 0.1
#define MAX_MESSAGES 4
#define DB_PATH "craft.db"
#define USE_CACHE 1
#define DAY_LENGTH 600
#define INVERT_MOUSE 0

// rendering options
#define SHOW_LIGHTS 1
#define SHOW_ITEM 1
#define SHOW_CROSSHAIRS 1
#define SHOW_WIREFRAME 1
#define SHOW_INFO_TEXT 1
#define SHOW_CHAT_TEXT 1
#define SHOW_PLAYER_NAMES 1

#define CRAFT_KEY_SIGN '`'

// advanced parameters
#define CREATE_CHUNK_RADIUS 10
#define RENDER_CHUNK_RADIUS 10
#define RENDER_SIGN_RADIUS 4
#define DELETE_CHUNK_RADIUS 14
#define COMMIT_INTERVAL 5

#define MAX_CHUNKS 8192
#define MAX_PLAYERS 1
#define NUM_WORKERS 4
#define MAX_TEXT_LENGTH 256
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
    typedef struct {
        int chunk_size;
        bool show_trees;
        bool show_plants;
        bool show_clouds;
        bool fullscreen;
        bool color_mode_dark;
        bool capture_mouse;
        char outfile[64] = ".obj";
        int build_width;
        int build_height;
        int build_length;
        int seed;
        std::string algo;

        void reset_outfile() {
            for (auto i = 0; i < IM_ARRAYSIZE(outfile); ++i) {
                outfile[i] = '\0';
            }
            outfile[0] = '.';
            outfile[1] = 'o';
            outfile[2] = 'b';
            outfile[3] = 'j';
        }
    } DearImGuiHelper;

    struct Maze {
        std::string maze;
        std::mutex maze_mutx;
        void set_maze(const std::string& maze) {
            std::lock_guard<std::mutex> lock(maze_mutx);
            this->maze = maze;
        };
        std::string get_maze() {
            std::lock_guard<std::mutex> lock(maze_mutx);
            return maze;
        }
    };

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
        thrd_t tiny_thrd;
        mtx_t tiny_mtx;
        cnd_t tiny_cnd;
        // SDL_Thread *thrd;
        // SDL_Mutex *mtx;
        // SDL_Condition *cnd;
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
        int text_len;
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
    
    struct Vertex {
        GLfloat x, y, z;
    };

    struct Face {
        std::vector<GLuint> vertices;
    };

    // Note: These are public members
    const std::string& m_window_name;
    const std::string& m_version;
    const std::string& m_help;

    unique_ptr<Model> m_model;

    DearImGuiHelper m_gui;

    vector<Vertex> m_vertices;
    vector<Face> m_faces;

    craft_impl(const std::string& window_name, const std::string& version, const std::string& help)
        : m_window_name{ window_name }
        , m_version{ version }
        , m_help{help}
        , m_model{make_unique<Model>()}
        // chunk_size, show_trees, show_plants, show_clouds, fullscreen, 
        // (continued)... color_mode_dark, capture_mouse, build_width, 
        // (continued)... build_height, build_length, seed, algo
        , m_gui{32, true, true, true, false, true, false, "my.obj", 46, 5, 10, 50, "binary_tree"}
        , m_vertices{}
        , m_faces{} {

    }

    static int worker_run(void *arg) {
        Worker *worker = (Worker *)arg;
        
        craft caller{ {}, {}, {} };
        while (1) {
            // SDL_LockMutex(worker->mtx);
            mtx_lock(&worker->tiny_mtx);
            while (worker->state != WORKER_BUSY && !worker->should_stop) {
                cnd_wait(&worker->tiny_cnd, &worker->tiny_mtx);
                // SDL_WaitCondition(worker->cnd, worker->mtx);
            }
            if (worker->should_stop) {
                mtx_unlock(&worker->tiny_mtx);
				// SDL_UnlockMutex(worker->mtx);
				break;
			}
            // SDL_UnlockMutex(worker->mtx);
            mtx_unlock(&worker->tiny_mtx);
            WorkerItem *worker_item = &worker->item;
            if (worker_item->load) {
                caller.m_pimpl->load_chunk(worker_item);
            }
            
            caller.m_pimpl->compute_chunk(worker_item);
            // SDL_LockMutex(worker->mtx);
            mtx_lock(&worker->tiny_mtx);
            worker->state = WORKER_DONE;
            mtx_unlock(&worker->tiny_mtx);
            // SDL_UnlockMutex(worker->mtx);
        }
        return 0;
    } // worker_run


    void init_worker_threads() {
        this->m_model->workers.reserve(NUM_WORKERS);
        for (int i = 0; i < NUM_WORKERS; i++) {
            auto worker = make_unique<Worker>();
            worker->index = i;
            worker->state = WORKER_IDLE;
//             SDL_Mutex *sdl_mtx = SDL_CreateMutex();
//             if (sdl_mtx == nullptr) {
// #if defined(MAZE_DEBUG)
//                 SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create mutex: %s", SDL_GetError());
// #endif
//                 return;
//             }
//             worker->mtx = sdl_mtx;
//             SDL_Condition *sdl_cnd = SDL_CreateCondition();
//             if (sdl_cnd == nullptr) {
// #if defined(MAZE_DEBUG)
//                 SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create condition variable: %s", SDL_GetError());
// #endif            	
// 				return;
//             }
//             worker->cnd = sdl_cnd;
//             worker->thrd = SDL_CreateThread(worker_run, "worker thread", worker.get());
            mtx_init(&worker->tiny_mtx, mtx_plain);
            cnd_init(&worker->tiny_cnd);
            thrd_create(&worker->tiny_thrd, worker_run, worker.get());
            this->m_model->workers.emplace_back(std::move(worker));
        }
    }

    /**
     * Cleanup the worker threads
     */
    void cleanup_worker_threads() {
        // signal all worker threads to stop
        for (auto&& w : this->m_model->workers) {
            // SDL_LockMutex(w->mtx);
            mtx_lock(&w->tiny_mtx);
            w->should_stop = true;
            cnd_signal(&w->tiny_cnd);
            mtx_unlock(&w->tiny_mtx);
            // SDL_SignalCondition(w->cnd);
            // SDL_UnlockMutex(w->mtx);
        }
        // Wait for threads to join
        for (auto&& w : this->m_model->workers) {
            // Wait for the thread to complete its execution
            int threadReturnValue = -1;
            thrd_join(w->tiny_thrd, &threadReturnValue);
            // SDL_WaitThread(w->thrd, &threadReturnValue);
#if defined(MAZE_DEBUG)
            SDL_Log("Worker thread finished with return value: %d", threadReturnValue);
#endif
            // Clean up the mutex and condition variable
            // SDL_DestroyMutex(w->mtx);
            // SDL_DestroyCondition(w->cnd);
            mtx_destroy(&w->tiny_mtx);
            cnd_destroy(&w->tiny_cnd);
        }
        // Clear the vector after all threads have been joined
        this->m_model->workers.clear();
    }

    void del_buffer(GLuint buffer) {
        glDeleteBuffers(1, &buffer);
    }

    GLuint gen_buffer(GLsizei size, GLfloat *data) {
        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return buffer;
    }

    GLfloat *malloc_faces(int components, int faces) {
        return (GLfloat*) malloc(sizeof(GLfloat) * 6 * components * faces);
    }

    GLuint gen_faces(int components, int faces, GLfloat *data) {
        GLuint buffer = this->gen_buffer(sizeof(GLfloat) * 6 * components * faces, data);
        free(data);
        return buffer;
    }

    int chunked(float x) const {
        return SDL_floorf(SDL_roundf(x) / this->m_gui.chunk_size);
    }
    double get_time() const {
    	return (SDL_GetTicks() + (double) this->m_model->start_time - (double) this->m_model->start_ticks) / 1000.0;
    }
    float time_of_day() const {
        if (this->m_model->day_length <= 0) {
            return 0.5;
        }
        float t;
        t = get_time();
        t = t / this->m_model->day_length;
        t = t - (int)t;
        return t;
    }

    float get_daylight() const {
        float timer = time_of_day();
        if (timer < 0.5) {
            float t = (timer - 0.25) * 100;
            return 1 / (1 + SDL_powf(2, -t));
        }
        else {
            float t = (timer - 0.85) * 100;
            return 1 - 1 / (1 + SDL_powf(2, -t));
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
        *vx = SDL_cosf(rx - RADIANS(90)) * m;
        *vy = SDL_sinf(ry);
        *vz = SDL_sinf(rx - RADIANS(90)) * m;
    }

    void get_motion_vector(int flying, int sz, int sx, float rx, float ry,
        float *vx, float *vy, float *vz) const {
        *vx = 0; *vy = 0; *vz = 0;
        if (!sz && !sx) {
            return;
        }
        float strafe = SDL_atan2f(sz, sx);
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
        int length = strlen(text);
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

    void draw_triangles_2d(Attrib *attrib, GLuint buffer, int count) {
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

    void draw_text(Attrib *attrib, GLuint buffer, int length) {
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
            memcpy(s1, s2, sizeof(State));
            s2->x = x; s2->y = y; s2->z = z; s2->rx = rx; s2->ry = ry;
            s2->t = get_time();
            if (s2->rx - s1->rx > PI) {
                s1->rx += 2 * PI;
            }
            if (s1->rx - s2->rx > PI) {
                s1->rx -= 2 * PI;
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
        float t1 = s2->t - s1->t;
        float t2 = this->get_time() - s2->t;
        t1 = SDL_min(t1, 1);
        t1 = SDL_max(t1, 0.1);
        float p = SDL_min(t2 / t1, 1);
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
        memcpy(player, other, sizeof(Player));
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
        return SDL_sqrt(x * x + y * y + z * z);
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
        return SDL_sqrt(x * x + y * y + z * z);
    }

    Player *player_crosshair(Player *player) {
        Player *result = 0;
        float threshold = RADIANS(5);
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

    Chunk *find_chunk(int p, int q) {
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
        float x = static_cast<float>(p * this->m_gui.chunk_size - 1);
        float z = static_cast<float>(q * this->m_gui.chunk_size - 1);
        float d = static_cast<float>(this->m_gui.chunk_size + 1);
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

    int highest_block(float x, float z) {
        int result = -1;
        int nx = SDL_roundf(x);
        int nz = SDL_roundf(z);
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
            int nx = SDL_roundf(x);
            int ny = SDL_roundf(y);
            int nz = SDL_roundf(z);
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
                float d = SDL_sqrt(SDL_powf(hx - x, 2) + SDL_powf(hy - y, 2) + SDL_powf(hz - z, 2));
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
                int degrees = SDL_roundf(DEGREES(SDL_atan2f(s->x - hx, s->z - hz)));
                if (degrees < 0) {
                    degrees += 360;
                }
                int top = ((degrees + 45) / 90) % 4;
                *face = 4 + top; return 1;
            }
        }
        return 0;
    }

    int collide(int height, float *x, float *y, float *z) {
        int result = 0;
        int p = this->chunked(*x);
        int q = this->chunked(*z);
        Chunk *chunk = this->find_chunk(p, q);
        if (!chunk) {
            return result;
        }
        Map *map = &chunk->map;
        int nx = SDL_roundf(*x);
        int ny = SDL_roundf(*y);
        int nz = SDL_roundf(*z);
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

    int player_intersects_block(int height, float x, float y, float z, int hx, int hy, int hz) {
        int nx = SDL_roundf(x);
        int ny = SDL_roundf(y);
        int nz = SDL_roundf(z);
        for (int i = 0; i < height; i++) {
            if (nx == hx && ny - i == hy && nz == hz) {
                return 1;
            }
        }
        return 0;
    }

    int _gen_sign_buffer(GLfloat *data, float x, float y, float z, int face, const char *text) {
        static constexpr int glyph_dx[8] = {0, 0, -1, 1, 1, 0, -1, 0};
        static constexpr int glyph_dz[8] = {1, -1, 0, 0, 0, -1, 0, 1};
        static constexpr int line_dx[8] = {0, 0, 0, 0, 0, 1, 0, -1};
        static constexpr int line_dy[8] = {-1, -1, -1, -1, 0, 0, 0, 0};
        static constexpr int line_dz[8] = {0, 0, 0, 0, 1, 0, -1, 0};
        if (face < 0 || face >= 8) {
            return 0;
        }
        int count = 0;
        float max_width = 64;
        float line_height = 1.25;
        char lines[1024];
        int rows = wrap(text, max_width, lines, 1024);
        rows = SDL_min(rows, 5);
        int dx = glyph_dx[face];
        int dz = glyph_dz[face];
        int ldx = line_dx[face];
        int ldy = line_dy[face];
        int ldz = line_dz[face];
        float n = 1.0 / (max_width / 10);
        float sx = x - n * (rows - 1) * (line_height / 2) * ldx;
        float sy = y - n * (rows - 1) * (line_height / 2) * ldy;
        float sz = z - n * (rows - 1) * (line_height / 2) * ldz;
        char *key;
        // util.h -> tokenize
        char *line = tokenize(lines, "\n", &key);
        while (line) {
            int length = strlen(line);
            int line_width = string_width(line);
            line_width = SDL_min(line_width, max_width);
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

    void gen_sign_buffer(Chunk *chunk) {
        SignList *signs = &chunk->signs;

        // first pass - count characters
        int max_faces = 0;
        for (int i = 0; i < signs->size; i++) {
            Sign *e = signs->data + i;
            max_faces += strlen(e->text);
        }

        // second pass - generate geometry
        GLfloat *data = malloc_faces(5, max_faces);
        int faces = 0;
        for (int i = 0; i < signs->size; i++) {
            Sign *e = signs->data + i;
            faces += this->_gen_sign_buffer(data + faces * 30, e->x, e->y, e->z, e->face, e->text);
        }

        this->del_buffer(chunk->sign_buffer);
        chunk->sign_buffer = this->gen_faces(5, faces, data);
        chunk->sign_faces = faces;
    }

    int has_lights(Chunk *chunk) {
        if (!SHOW_LIGHTS) {
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

    void dirty_chunk(Chunk *chunk) {
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

    void occlusion(char neighbors[27], char lights[27], float shades[27], float ao[6][4], float light[6][4]) {
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
                float total = curve[value] + shade_sum / 4.0;
                ao[i][j] = SDL_min(total, 1.0);
                light[i][j] = light_sum / 15.0 / 4.0;
            }
        }
    } // occlusion

    void light_fill(char *opaque, char *light, int x, int y, int z, int w, int force) {
#define XZ_SIZE (this->m_gui.chunk_size * 3 + 2)
#define XZ_LO (this->m_gui.chunk_size)
#define XZ_HI (this->m_gui.chunk_size * 2 + 1)
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
    void compute_chunk(WorkerItem *item) {
        char *opaque = (char *)calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
        char *light = (char *)calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
        char *highest = (char *)calloc(XZ_SIZE * XZ_SIZE, sizeof(char));

        int ox = item->p * this->m_gui.chunk_size - this->m_gui.chunk_size - 1;
        int oy = -1;
        int oz = item->q * this->m_gui.chunk_size - this->m_gui.chunk_size - 1;

        // check for lights
        int has_light = 0;
        if (SHOW_LIGHTS) {
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
                Map *map = item->block_maps[a][b];
                if (!map) {
                    continue;
                }
                MAP_FOR_EACH(map, ex, ey, ez, ew) {
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

        Map *map = item->block_maps[1][1];

        // count exposed faces
        int miny = 256;
        int maxy = 0;
        int faces = 0;
        MAP_FOR_EACH(map, ex, ey, ez, ew) {
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
        static constexpr int components = 10;
        GLfloat *data = malloc_faces(components, faces);
        int offset = 0;
        MAP_FOR_EACH(map, ex, ey, ez, ew) {
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
                                    shades[index] = 1.0 - oy * 0.125;
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
                float rotation = simplex2(ex, ez, 4, 0.5, 2) * 360;
                make_plant(
                    data + offset, min_ao, max_light,
                    ex, ey, ez, 0.5, ew, rotation);
            }
            else {
                make_cube(
                    data + offset, ao, light,
                    f1, f2, f3, f4, f5, f6,
                    ex, ey, ez, 0.5, ew);
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

    void generate_chunk(Chunk *chunk, WorkerItem *item) {
        chunk->miny = item->miny;
        chunk->maxy = item->maxy;
        chunk->faces = item->faces;
        this->del_buffer(chunk->buffer);
        chunk->buffer = this->gen_faces(10, item->faces, item->data);
        this->gen_sign_buffer(chunk);
    }

    void gen_chunk_buffer(Chunk *chunk) {
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
        int p = item->p;
        int q = item->q;
        Map *block_map = item->block_maps[1][1];
        Map *light_map = item->light_maps[1][1];
        // world.h
        static world _world;
        auto&& gui_options = this->m_gui;
        _world.create_world(p, q, map_set_func, block_map, gui_options.chunk_size, gui_options.show_trees, gui_options.show_plants, gui_options.show_clouds);
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
        int dx = p * this->m_gui.chunk_size - 1;
        int dy = 0;
        int dz = q * this->m_gui.chunk_size - 1;
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
                memcpy(chunk, other, sizeof(Chunk));
            }
        }
        this->m_model->chunk_count = count;
    }

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
            // SDL_LockMutex(worker->mtx);
            mtx_lock(&worker->tiny_mtx);
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
                            free(block_map);
                        }
                        if (light_map) {
                            map_free(light_map);
                            free(light_map);
                        }
                    }
                }
                worker->state = WORKER_IDLE;
            }
            // SDL_UnlockMutex(worker->mtx);
            mtx_unlock(&worker->tiny_mtx);
        }
    }

    // Used to init the terrain (chunks) around the player
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
     * @brief set_vertex_data parses the grid, and builds a 3D grid using grid row, column, height
     * Calling set_vertex_data, set_block, record_block to DB
     * Specify a "starting_height" to try to put the maze above the heightmap (mountains)
     * @param height of the grid
     * @param grid_as_ascii
     * @return
     */
    bool set_vertex_data(unsigned int height, const string& grid_as_ascii)  {
        auto add_block_to_vertex_data = [this](float x, float y, float z, float block_size) {
            // Calculate the base index for the new vertices
            unsigned int baseIndex = this->m_vertices.size() + 1; // OBJ format is 1-based indexing

            // Define the 8 vertices of the cube
            this->m_vertices.push_back({ x, y, z });
            this->m_vertices.push_back({ x + block_size, y, z });
            this->m_vertices.push_back({ x + block_size, y + block_size, z });
            this->m_vertices.push_back({ x, y + block_size, z });
            this->m_vertices.push_back({ x, y, z + block_size });
            this->m_vertices.push_back({ x + block_size, y, z + block_size });
            this->m_vertices.push_back({ x + block_size, y + block_size, z + block_size });
            this->m_vertices.push_back({ x, y + block_size, z + block_size });

            // Define faces using the vertices above (12 triangles for 6 faces)
            // Front face
            this->m_faces.push_back({ {baseIndex, baseIndex + 1, baseIndex + 2} });
            this->m_faces.push_back({ {baseIndex, baseIndex + 2, baseIndex + 3} });
            // Back face
            this->m_faces.push_back({ {baseIndex + 4, baseIndex + 6, baseIndex + 5} });
            this->m_faces.push_back({ {baseIndex + 4, baseIndex + 7, baseIndex + 6} });
            // Left face
            this->m_faces.push_back({ {baseIndex, baseIndex + 3, baseIndex + 7} });
            this->m_faces.push_back({ {baseIndex, baseIndex + 7, baseIndex + 4} });
            // Right face
            this->m_faces.push_back({ {baseIndex + 1, baseIndex + 5, baseIndex + 6} });
            this->m_faces.push_back({ {baseIndex + 1, baseIndex + 6, baseIndex + 2} });
            // Top face
            this->m_faces.push_back({ {baseIndex + 3, baseIndex + 2, baseIndex + 6} });
            this->m_faces.push_back({ {baseIndex + 3, baseIndex + 6, baseIndex + 7} });
            // Bottom face
            this->m_faces.push_back({ {baseIndex, baseIndex + 4, baseIndex + 5} });
            this->m_faces.push_back({ {baseIndex, baseIndex + 5, baseIndex + 1} });
        }; // add_block_to_vertex_data


            istringstream iss{ grid_as_ascii.data() };
            string line;
            unsigned int row_x = 0;
            while (getline(iss, line, '\n')) {
                unsigned int col_z = 0;
                for (auto itr = line.cbegin(); itr != line.cend() && col_z < line.size(); itr++) {
                    if (*itr == ' ') {
                        col_z++;
                    } else if (*itr == '+' || *itr == '-' || *itr == '|') {
                        // check for barriers and walls then iterate up/down
                        static constexpr unsigned int starting_height = 30u;
                        static constexpr float block_size = 1.0f;
                        for (auto h{ starting_height }; h < starting_height + height; h++) {
                            int w = items[this->m_model->item_index];
                            // set the block in the craft
                            set_block(row_x, h, col_z, w);
                            record_block(row_x, h, col_z, w);
                            // update the data source that stores the grid for writing to file
                            add_block_to_vertex_data(row_x, h, col_z, block_size);
                        }
                        col_z++;
                    }
                }

                row_x++;
            } // getline
        return true;
    } // set_vertex_data

    /**
     * @brief Generate a grid containing a maze in ASCII format which will be used to write and display the maze
     * @return
     */
    std::string gen_maze(const function<int(int, int)>& get_int, const std::mt19937& rng,
        const function<maze_types(const string& algo)> get_maze_algo_from_str, const string& current_maze_algo) const noexcept {

        mazes::maze_types my_maze_type = get_maze_algo_from_str(current_maze_algo);

        auto _grid{ std::make_unique<mazes::grid>(this->m_gui.build_width, this->m_gui.build_length, this->m_gui.build_height) };

	    bool success = mazes::maze_factory::gen_maze(my_maze_type, ref(_grid), cref(get_int), cref(rng));

        if (!success) {
            return "";
        }

        stringstream ss;
        ss << *_grid.get();
        string grid_as_ascii{ ss.str() };

        return grid_as_ascii;
    }

    // Return true when maze has been written
    future<bool> write_maze(const string& filename) {
        // helper lambda to write the Wavefront object file
        auto convert_data_to_mesh_str = [this](const vector<craft_impl::Vertex>& vertices, const vector<craft_impl::Face>& faces) {
            stringstream ss;
            ss << "# https://www.github.com/zmertens/MazeBuilder\n";

            // keep track of writing progress
            int total_verts = vertices.size();
            int total_faces = faces.size();
            
            int t = total_verts + total_faces;
            int c = 0;
            // Write vertices
            for (const auto& vertex : vertices) {
                ss << "v " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
                c++;
            }

            // Write faces
            for (const auto& face : faces) {
                ss << "f";
                for (auto index : face.vertices) {
                    ss << " " << index;
                }
                ss << "\n";
                c++;
            }

#if defined(MAZE_DEBUG)
            SDL_Log("Writing maze progress: %d/%d", c, t);
#endif

            return ss.str();
        }; // convert_data_to_mesh_str

        if (!filename.empty()) {
            // get ready to write to file
            writer maze_writer;
            packaged_task<bool(const string& out_file)> maze_writing_task{ [this, &maze_writer, &convert_data_to_mesh_str](auto out)->bool {
                return maze_writer.write(out, convert_data_to_mesh_str(this->m_vertices, this->m_faces)); }};
            auto writing_results = maze_writing_task.get_future();
            thread(std::move(maze_writing_task), filename ).detach();
            return writing_results;
        } else {
            promise<bool> p;
            p.set_value(false);
            return p.get_future();
        }
    } // write_maze

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
        // SDL_SignalCondition(worker->cnd);
        cnd_signal(&worker->tiny_cnd);
    } // ensure chunks worker

    void ensure_chunks(Player *player) {
        check_workers();
        force_chunks(player);
        for (auto&& worker : this->m_model->workers) {
            // SDL_LockMutex(worker->mtx);
            mtx_lock(&worker->tiny_mtx);
            if (worker->state == WORKER_IDLE) {
                ensure_chunks_worker(player, worker.get());
            }
            // SDL_UnlockMutex(worker->mtx);
            mtx_unlock(&worker->tiny_mtx);
        }
    }

    void unset_sign(int x, int y, int z) {
        int p = chunked(x);
        int q = chunked(z);
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

    void unset_sign_face(int x, int y, int z, int face) {
        int p = chunked(x);
        int q = chunked(z);
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

    void _set_sign(int p, int q, int x, int y, int z, int face, const char *text, int dirty) {
        if (strlen(text) == 0) {
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

    void set_sign(int x, int y, int z, int face, const char *text) {
        int p = chunked(x);
        int q = chunked(z);
        _set_sign(p, q, x, y, z, face, text, 1);
    }

    void toggle_light(int x, int y, int z) {
        int p = chunked(x);
        int q = chunked(z);
        Chunk *chunk = find_chunk(p, q);
        if (chunk) {
            Map *map = &chunk->lights;
            int w = map_get(map, x, y, z) ? 0 : 15;
            map_set(map, x, y, z, w);
            db_insert_light(p, q, x, y, z, w);
            dirty_chunk(chunk);
        }
    }

    void set_light(int p, int q, int x, int y, int z, int w) {
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

    void _set_block(int p, int q, int x, int y, int z, int w, int dirty) {
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
        if (w == 0 && chunked(x) == p && chunked(z) == q) {
            unset_sign(x, y, z);
            set_light(p, q, x, y, z, 0);
        }
    }

    void set_block(int x, int y, int z, int w) {
        int p = chunked(x);
        int q = chunked(z);
        _set_block(p, q, x, y, z, w, 1);
        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                if (dx == 0 && dz == 0) {
                    continue;
                }
                if (dx && chunked(x + dx) == p) {
                    continue;
                }
                if (dz && chunked(z + dz) == q) {
                    continue;
                }
                _set_block(p + dx, q + dz, x, y, z, -w, 1);
            }
        }
    }

    void record_block(int x, int y, int z, int w) {
        SDL_memcpy(&this->m_model->block1, &this->m_model->block0, sizeof(Block));
        this->m_model->block0.x = x;
        this->m_model->block0.y = y;
        this->m_model->block0.z = z;
        this->m_model->block0.w = w;
    }

    int get_block(int x, int y, int z) {
        int p = chunked(x);
        int q = chunked(z);
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
        glUniform1f(attrib->extra3, this->m_model->render_radius * this->m_gui.chunk_size);
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
        int length = _gen_sign_buffer(data, x, y, z, face, text);
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
            // glEnable(GL_COLOR_LOGIC_OP);
            glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
            GLuint wireframe_buffer = gen_wireframe_buffer(hx, hy, hz, 0.53);
            draw_lines(attrib, wireframe_buffer, 3, 24);
            del_buffer(wireframe_buffer);
            // glDisable(GL_COLOR_LOGIC_OP);
        }
    }

    void render_crosshairs(Attrib *attrib) {
        float matrix[16];
        set_matrix_2d(matrix, this->m_model->width, this->m_model->height);
        glUseProgram(attrib->program);
        glLineWidth(4 * this->m_model->scale);
        // glEnable(GL_COLOR_LOGIC_OP);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        GLuint crosshair_buffer = gen_crosshair_buffer();
        draw_lines(attrib, crosshair_buffer, 2, 4);
        del_buffer(crosshair_buffer);
        // glDisable(GL_COLOR_LOGIC_OP);
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
        int length = strlen(text);
        x -= n * justify * (length - 1) / 2;
        GLuint buffer = gen_text_buffer(x, y, n, text);
        draw_text(attrib, buffer, length);
        del_buffer(buffer);
    }

    void add_message(const char *text) {
        printf("%s\n", text);
        snprintf(
            this->m_model->messages[this->m_model->message_index], MAX_TEXT_LENGTH, "%s", text);
        this->m_model->message_index = (this->m_model->message_index + 1) % MAX_MESSAGES;
    }

    void copy() {
        memcpy(&this->m_model->copy0, &this->m_model->block0, sizeof(Block));
        memcpy(&this->m_model->copy1, &this->m_model->block1, sizeof(Block));
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
                        float d = SDL_sqrt(dx * dx + dy * dy + dz * dz);
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

    void parse_command(const char *buffer, int forward) {
        char username[128] = {0};
        char token[128] = {0};
        char filename[MAX_PATH_LENGTH];
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
    }

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
    * This function also checks if user is interacting with gui to prevent mouse hiding
    * @param dt
    * @param running reference to running loop in game loop
    * @return bool return true when event handle successfully
    */
    bool handle_events(double dt, bool& running) {
        static float dy = 0;
        State* s = &this->m_model->players->state;
        int sz = 0;
        int sx = 0;
        float m = 0.0025;
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
                    case SDL_SCANCODE_INSERT:
                        if (this->m_model->typing) {
                            this->m_model->typing = 0;
                        }
                    }
                    break;
                }
                case SDL_EVENT_KEY_DOWN: {
                    
                    sc = e.key.scancode;
                    switch (sc) {
                    case SDL_SCANCODE_ESCAPE: {
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        this->m_gui.capture_mouse = false;
                        this->m_gui.fullscreen = false;
                        break;
                    }
                    case SDL_SCANCODE_RETURN: {
                        if (this->m_model->typing) {
                            if (mod_state /*& (KMOD_LSHIFT | KMOD_RSHIFT)*/) {
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
                                    this->parse_command(this->m_model->typing_buffer, 1);
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
                            char* clip_buffer = SDL_GetClipboardText();
                            if (this->m_model->typing) {
                                this->m_model->suppress_char = 1;
                                SDL_strlcat(this->m_model->typing_buffer, clip_buffer,
                                    MAX_TEXT_LENGTH - this->m_model->text_len - 1);
                            } else {
                                parse_command(clip_buffer, 0);
                            }
                        }
                        break;
                    }
                    case SDL_SCANCODE_0: {
                        if (!this->m_model->typing)
                            this->m_model->item_index = 9;
                        break;
                    }
                    case SDL_SCANCODE_1: [[fallthrough]];
                    case SDL_SCANCODE_2: [[fallthrough]];
                    case SDL_SCANCODE_3: [[fallthrough]];
                    case SDL_SCANCODE_4: [[fallthrough]];
                    case SDL_SCANCODE_5: [[fallthrough]];
                    case SDL_SCANCODE_6: [[fallthrough]];
                    case SDL_SCANCODE_7: [[fallthrough]];
                    case SDL_SCANCODE_8: [[fallthrough]];
                    case SDL_SCANCODE_9: {
                        if (this->m_gui.capture_mouse && !this->m_model->typing)
                            this->m_model->item_index = (sc - SDL_SCANCODE_1);
                        break;
                    }
                    case KEY_FLY: {
                        if (!this->m_model->typing && this->m_gui.capture_mouse)
                            this->m_model->flying = ~this->m_model->flying;
                        break;
                    }
                    case KEY_ITEM_NEXT: {
                        if (!this->m_model->typing)
                            this->m_model->item_index = (this->m_model->item_index + 1) % item_count;
                        break;
                    }
                    case KEY_ITEM_PREV: {
                        if (!this->m_model->typing) {
                            this->m_model->item_index--;
                            if (this->m_model->item_index < 0)
                                this->m_model->item_index = item_count - 1;
                        }
                        break;
                    }
                    case KEY_OBSERVE: {
                        if (!this->m_model->typing)
                            this->m_model->observe1 = (this->m_model->observe1 + 1) % this->m_model->player_count;
                        break;
                    }
                    case KEY_OBSERVE_INSET: {
                        if (!this->m_model->typing)
                            this->m_model->observe2 = (this->m_model->observe2 + 1) % this->m_model->player_count;
                        break;
                    }
                    case KEY_CHAT: {
                        this->m_model->typing = 1;
                        this->m_model->typing_buffer[0] = '\0';
                        this->m_model->text_len = 0;
                        SDL_StartTextInput(this->m_model->window);
                        break;
                    }
                    case KEY_COMMAND: {
                        this->m_model->typing = 1;
                        this->m_model->typing_buffer[0] = '\0';
                        SDL_StartTextInput(this->m_model->window);
                        break;
                    }
                    case KEY_SIGN: {
                        this->m_model->typing = 1;
                        this->m_model->typing_buffer[0] = '\0';
                        SDL_StartTextInput(this->m_model->window);
                        break;

                    }
                    } // switch
                    break;
                } // case SDL_EVENT_KEY_DOWN
                case SDL_EVENT_TEXT_INPUT: {
                    // could probably just do text[text_len++] = e.text.text[0]
                    // since I only handle ascii
                    if (this->m_gui.capture_mouse && this->m_model->typing && this->m_model->text_len < MAX_TEXT_LENGTH - 1) {
                        strcat(this->m_model->typing_buffer, e.text.text);
                        this->m_model->text_len += SDL_strlen(e.text.text);
                        //SDL_Log("text is \"%s\" \"%s\" %d %d\n", this->m_model->typing_buffer, composition, cursor, selection_len);
                        //SDL_LogError(SDL_LOG_CATEGORY_ERROR, "text is \"%s\" \"%s\" %d %d\n", text, composition, cursor, selection_len);
                    }
                    break;
                }
                case SDL_EVENT_MOUSE_MOTION: {
                    if (this->m_gui.capture_mouse && SDL_GetRelativeMouseMode()) {
                        s->rx += e.motion.xrel * m;
                        if (INVERT_MOUSE) {
                            s->ry += e.motion.yrel * m;
                        }
                        else {
                            s->ry -= e.motion.yrel * m;
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
                    if (this->m_gui.capture_mouse && SDL_GetRelativeMouseMode() && e.button.button == SDL_BUTTON_LEFT) {
                        if (control) {
                            on_right_click();
                        }
                        else {
                            on_left_click();
                        }
                    }
                    else if (this->m_gui.capture_mouse && SDL_GetRelativeMouseMode() && e.button.button == SDL_BUTTON_RIGHT) {
                        if (control) {
                            on_light();
                        }
                        else {
                            on_right_click();
                        }
                    }
                    else if (e.button.button == SDL_BUTTON_MIDDLE) {
                        if (this->m_gui.capture_mouse && SDL_GetRelativeMouseMode()) {
                            on_middle_click();
                        }
                    }

                    break;
                }
                case SDL_EVENT_MOUSE_WHEEL: {
                    if (this->m_gui.capture_mouse && SDL_GetRelativeMouseMode()) {
                        // TODO might have to change this to force 1 step
                        if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL) {
                            this->m_model->item_index += e.wheel.y;
                        }
                        else {
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

        const Uint8 *state = SDL_GetKeyboardState(nullptr);

        if (!this->m_model->typing && this->m_gui.capture_mouse) {
            this->m_model->is_ortho = state[KEY_ORTHO] ? 64 : 0;
            this->m_model->fov = state[KEY_ZOOM] ? 15 : 65;
            if (state[KEY_FORWARD]) sz--;
            if (state[KEY_BACKWARD]) sz++;
            if (state[KEY_LEFT]) sx--;
            if (state[KEY_RIGHT]) sx++;
            if (state[SDL_SCANCODE_LEFT]) s->rx -= m;
            if (state[SDL_SCANCODE_RIGHT]) s->rx += m;
            if (state[SDL_SCANCODE_UP]) s->ry += m;
            if (state[SDL_SCANCODE_DOWN]) s->ry -= m;
        }

        float vx, vy, vz;
        get_motion_vector(this->m_model->flying, sz, sx, s->rx, s->ry, &vx, &vy, &vz);
        if (!this->m_model->typing) {
            if (state[KEY_JUMP] && this->m_gui.capture_mouse) {
                if (this->m_model->flying) {
                    vy = 1;
                }
                else if (dy == 0) {
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
            }
            else {
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
     * @brief Check fullscreen modes avaialble and update the model
     */
    void set_model_using_fullscreen_modes() {
        if (this->m_gui.fullscreen) {
            SDL_DisplayID display = SDL_GetPrimaryDisplay();
            int num_modes = 0;
            const SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(display, &num_modes);
            if (modes) {
                for (int i = 0; i < num_modes; ++i) {
                    const SDL_DisplayMode* mode = modes[i];
#if defined(MAZE_DEBUG)
                    SDL_Log("Display %" SDL_PRIu32 " mode %d: %dx%d@%gx %gHz\n", display, i, mode->w, mode->h, mode->pixel_density, mode->refresh_rate);
#endif
                }
                this->m_model->width = modes[num_modes - 1]->w;
                this->m_model->height = modes[num_modes - 1]->h;

                SDL_free(modes);
            }
        }
    }

    /**
     * @brief Create SDL/GL window and context, check display modes
     */
    void create_window_and_context() {
        this->m_model->start_ticks = static_cast<int>(SDL_GetTicks());
        Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
        int window_width = WINDOW_WIDTH;
        int window_height = WINDOW_HEIGHT;

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

        SDL_GL_SetSwapInterval(VSYNC);

        auto icon_path{ "textures/maze_in_green_32x32.bmp" };
        SDL_Surface* icon_surface = SDL_LoadBMP(icon_path);
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
bool craft::run(unsigned long seed, const std::list<std::string>& algos,
    const std::function<mazes::maze_types(const std::string& algo)> get_maze_algo_from_str) const noexcept {
    // Init RNG engine
    std::mt19937 rng_machine{ seed };
    this->m_pimpl->m_gui.seed = seed;
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
    SDL_SetRelativeMouseMode(SDL_FALSE);

#if !defined(__EMSCRIPTEN__)
    if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "OpenGL loader failed (%s)\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
#endif

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
    block_attrib.position = glGetAttribLocation(program, "position");
    block_attrib.normal = glGetAttribLocation(program, "normal");
    block_attrib.uv = glGetAttribLocation(program, "uv");
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
    line_attrib.position = glGetAttribLocation(program, "position");
    line_attrib.matrix = glGetUniformLocation(program, "matrix");

#if defined(__EMSCRIPTEN__)
    program = load_program("shaders/es/text_vertex.es.glsl", "shaders/es/text_fragment.es.glsl");
#else
    program = load_program("shaders/text_vertex.glsl", "shaders/text_fragment.glsl");
#endif
    text_attrib.program = program;
    text_attrib.position = glGetAttribLocation(program, "position");
    text_attrib.uv = glGetAttribLocation(program, "uv");
    text_attrib.matrix = glGetUniformLocation(program, "matrix");
    text_attrib.sampler = glGetUniformLocation(program, "sampler");
    text_attrib.extra1 = glGetUniformLocation(program, "is_sign");

#if defined(__EMSCRIPTEN__)
    program = load_program("shaders/es/sky_vertex.es.glsl", "shaders/es/sky_fragment.es.glsl");
#else
    program = load_program("shaders/sky_vertex.glsl", "shaders/sky_fragment.glsl");
#endif
    sky_attrib.program = program;
    sky_attrib.position = glGetAttribLocation(program, "position");
    sky_attrib.normal = 1; // glGetAttribLocation(program, "normal");
    sky_attrib.uv = glGetAttribLocation(program, "uv");
    sky_attrib.matrix = glGetUniformLocation(program, "matrix");
    sky_attrib.sampler = glGetUniformLocation(program, "sampler");
    sky_attrib.timer = glGetUniformLocation(program, "timer");

    snprintf(m_pimpl->m_model->db_path, MAX_PATH_LENGTH, "%s", DB_PATH);

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

    ImFont *nunito_sans_font = io.Fonts->AddFontFromMemoryCompressedTTF(NunitoSans_compressed_data, NunitoSans_compressed_size, 18.f);

#if defined(MAZE_DEBUG)
    IM_ASSERT(nunito_sans_font != nullptr);
#endif
    
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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
            return -1;
        }
    }

    // LOCAL VARIABLES 
    m_pimpl->reset_model();
    FPS fps = {0, 0, 0};
    double last_commit = SDL_GetTicks();

    GLuint sky_buffer = m_pimpl->gen_sky_buffer();

    craft_impl::Player *me = m_pimpl->m_model->players;
    craft_impl::State *s = &m_pimpl->m_model->players->state;
    me->id = 0;
    me->name[0] = '\0';
    me->buffer = 0;
    m_pimpl->m_model->player_count = 1;

    // magic variables to prevent black screen on load - configured in handle_events()
    this->m_pimpl->m_model->is_ortho = 0;
    this->m_pimpl->m_model->fov = 65;

    // LOAD STATE FROM DATABASE 
    int loaded = db_load_state(&s->x, &s->y, &s->z, &s->rx, &s->ry);

    m_pimpl->force_chunks(me);

    if (!loaded) {
        s->y = m_pimpl->highest_block(s->x, s->z) + 2;
    }

#if defined(MAZE_DEBUG)
    SDL_Log("check_for_gl_err() prior to event loop\n");
    check_for_gl_err();
#endif

    // BEGIN EVENT LOOP
    craft_impl::Maze maze{ "" };
    future<bool> write_success;
    auto progress_tracker = std::make_shared<craft::craft_impl::ProgressTracker>();
    bool running = true;
    int previous = SDL_GetTicks();
#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (running)
#endif
    {
        // update gl if viewport has change (example, going or leaving fullscreen mode)
        glViewport(0, 0, this->m_pimpl->m_model->width, this->m_pimpl->m_model->height);
        // FRAME RATE 
        if (m_pimpl->m_model->time_changed) {
            m_pimpl->m_model->time_changed = 0;
            last_commit = SDL_GetTicks();
            SDL_memset(&fps, 0, sizeof(fps));
        }
        update_fps(&fps);
        double now = SDL_GetTicks();
        double dt = (now - previous) / 1000.0;
        dt = SDL_min(dt, 0.2);
        dt = SDL_max(dt, 0.0);
        previous = now;

        // ImGui window variables
        static bool show_demo_window = false;
        static bool show_builder_gui = true;
        bool events_handled_success = m_pimpl->handle_events(dt, running);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Show the big demo window?
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        if (show_builder_gui) {
            ImGui::PushFont(nunito_sans_font);
            // GUI Title Bar
            ImGui::Begin(this->m_pimpl->m_version.data());
            // GUI Tabs
            ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
            if (ImGui::BeginTabBar("MyTabBar", tab_bar_flags)) {
                if (ImGui::BeginTabItem("Builder")) {
                    ImGui::Text("Builder settings");

                    static unsigned int MAX_MAZE_WIDTH = 100;
                    if (ImGui::SliderInt("Width", &this->m_pimpl->m_gui.build_width, 1, MAX_MAZE_WIDTH)) {

                    }
                    static unsigned int MAX_MAZE_LENGTH = 100;
                    if (ImGui::SliderInt("Length", &this->m_pimpl->m_gui.build_length, 1, MAX_MAZE_LENGTH)) {

                    }
                    static unsigned int MAX_MAZE_HEIGHT = 15;
                    if (ImGui::SliderInt("Height", &this->m_pimpl->m_gui.build_height, 1, MAX_MAZE_HEIGHT)) {
                      
                    }
                    static unsigned int MAX_SEED_VAL = 1'000'000;
                    if (ImGui::SliderInt("Seed", &this->m_pimpl->m_gui.seed, 1, MAX_SEED_VAL)) {
                        rng_machine.seed(static_cast<unsigned long>(this->m_pimpl->m_gui.seed));
                    }
                    ImGui::InputText("Outfile", &this->m_pimpl->m_gui.outfile[0], IM_ARRAYSIZE(this->m_pimpl->m_gui.outfile));
                    if (ImGui::TreeNode("Maze Generator")) {
                        auto preview{ this->m_pimpl->m_gui.algo.c_str() };
                        ImGui::NewLine();
                        ImGuiComboFlags combo_flags = ImGuiComboFlags_PopupAlignLeft;
                        if (ImGui::BeginCombo("algorithm", preview, combo_flags)) {
                            for (const auto& itr : algos) {
                                bool is_selected = (itr == this->m_pimpl->m_gui.algo);
                                if (ImGui::Selectable(itr.c_str(), is_selected)) {
                                    this->m_pimpl->m_gui.algo = itr;
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
                    if (this->m_pimpl->m_gui.outfile[0] != '.') {
                        if (ImGui::Button("Build!")) {
                            maze.set_maze(this->m_pimpl->gen_maze(cref(get_int), cref(rng_machine), cref(get_maze_algo_from_str), cref(this->m_pimpl->m_gui.algo)));
                        } else {
                            ImGui::SameLine();
                            ImGui::Text("Building maze... %s\n", this->m_pimpl->m_gui.outfile);
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

                    // Check if maze is available and then perform two sequential operations:
                    // 1. Set the blocks of the maze (this will update the class member variables storing vertex data
                    // 2. Write the maze to a Wavefront object file using the vertex data
                    if (!maze.get_maze().empty()) {
                        // set grid in craft - 3D world - update class members which store vertex data
                        bool success = this->m_pimpl->set_vertex_data(this->m_pimpl->m_gui.build_height, maze.get_maze());
                        if (success) {
                            progress_tracker->start();
                            write_success = this->m_pimpl->write_maze(this->m_pimpl->m_gui.outfile);
                            progress_tracker->stop();
                        } else {
                            ImGui::NewLine(); ImGui::NewLine();
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.83f, 0.015f, 1.0f));
                            ImGui::Text("Failed to set maze: %s", this->m_pimpl->m_gui.outfile);
                            ImGui::NewLine();
                            ImGui::PopStyleColor();
                        }
                        maze.set_maze("");
                    }

                    if (write_success.valid() && write_success.wait_for(chrono::seconds(0)) == future_status::ready) {
                        // call the writer future and get the result
                        bool success = write_success.get();
#if !defined(__EMSCRIPTEN__)
                        if (success) {
                            // Dont display a message on the web browser, let the web browser handle that
                            ImGui::NewLine();
                            ImGui::Text("Maze written to %s\n", this->m_pimpl->m_gui.outfile);
                            ImGui::NewLine();
                        } else {
                            ImGui::NewLine();
                            ImGui::Text("Failed to write maze: %s\n", this->m_pimpl->m_gui.outfile);
                            ImGui::NewLine();
                        }
#endif
                        maze.set_maze("");
                    }
                    if (progress_tracker) {
                        // Show progress when writing
                        ImGui::NewLine(); ImGui::NewLine();
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.008f, 0.83f, 0.015f, 1.0f));
                        ImGui::Text("Finished writing maze in %f seconds", progress_tracker->get_duration_in_seconds());
                        ImGui::NewLine();
                        ImGui::PopStyleColor();
                    }

                    // Reset should remove outfile name, clear vertex data for all generated mazes and remove them from the world
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.023f, 0.015f, 1.0f));
                    if (ImGui::Button("Reset")) {
                        this->m_pimpl->m_vertices.clear();
                        this->m_pimpl->m_faces.clear();
                        this->m_pimpl->m_gui.reset_outfile();
                        maze.set_maze("");
                    }
                    ImGui::PopStyleColor();
                    
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Graphics")) {
                    ImGui::Text("Graphic settings");
                    
                    ImGui::Checkbox("Dark Mode", &this->m_pimpl->m_gui.color_mode_dark);
                    if (this->m_pimpl->m_gui.color_mode_dark)
                        ImGui::StyleColorsDark();
                    else
                        ImGui::StyleColorsLight();
                    
                    // fullscreen off on launch
                    ImGui::Checkbox("Fullscreen (ESC to Exit)", &this->m_pimpl->m_gui.fullscreen);
                    if (this->m_pimpl->m_gui.fullscreen) {
                        SDL_SetWindowFullscreen(this->m_pimpl->m_model->window, SDL_TRUE);
                        //this->m_pimpl->set_model_using_fullscreen_modes();
                    } else {
                        SDL_SetWindowFullscreen(this->m_pimpl->m_model->window, SDL_FALSE);
                    }

                    SDL_GetWindowSizeInPixels(this->m_pimpl->m_model->window, &this->m_pimpl->m_model->width, &this->m_pimpl->m_model->height);
                    
                    ImGui::Checkbox("Capture Mouse (ESC to Uncapture)", &this->m_pimpl->m_gui.capture_mouse);
                    if (this->m_pimpl->m_gui.capture_mouse && !SDL_GetRelativeMouseMode()) {
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                        // Hide GUI when the user captures the mouse
                        show_builder_gui = false;
                    }
                                    
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
            ImGui::NewLine();
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
            ImGui::PopFont();
        } // show_builder_gui

        // When ESCAPE is pressed and the mouse is captured, the SDL_Event loop will catch and release the mouse
        // This action reveals the GUI anytime the user hits ESCAPE
        if (!this->m_pimpl->m_gui.capture_mouse && !show_builder_gui && !SDL_GetRelativeMouseMode()) {
            show_builder_gui = true;
        }

        // FLUSH DATABASE 
        if (now - last_commit > COMMIT_INTERVAL) {
            last_commit = now;
            db_commit();
        }
    
        // PREPARE TO RENDER 
        m_pimpl->m_model->observe1 = m_pimpl->m_model->observe1 % m_pimpl->m_model->player_count;
        m_pimpl->m_model->observe2 = m_pimpl->m_model->observe2 % m_pimpl->m_model->player_count;
    
        m_pimpl->delete_chunks();
        m_pimpl->del_buffer(me->buffer);
    
        me->buffer = m_pimpl->gen_player_buffer(s->x, s->y, s->z, s->rx, s->ry);
        for (int i = 1; i < m_pimpl->m_model->player_count; i++) {
            m_pimpl->interpolate_player(m_pimpl->m_model->players + i);
        }

        craft_impl::Player *player = m_pimpl->m_model->players + m_pimpl->m_model->observe1;

        // RENDER 3-D SCENE
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_pimpl->render_sky(&sky_attrib, player, sky_buffer);
        glClear(GL_DEPTH_BUFFER_BIT);

        int face_count = m_pimpl->render_chunks(&block_attrib, player);
        m_pimpl->render_signs(&text_attrib, player);
        m_pimpl->render_sign(&text_attrib, player);
        m_pimpl->render_players(&block_attrib, player);
        if (SHOW_WIREFRAME) {
            m_pimpl->render_wireframe(&line_attrib, player);
        }

        // RENDER HUD 
        glClear(GL_DEPTH_BUFFER_BIT);
        if (SHOW_CROSSHAIRS) {
            m_pimpl->render_crosshairs(&line_attrib);
        }
        if (SHOW_ITEM) {
            m_pimpl->render_item(&block_attrib);
        }

        // RENDER TEXT 
        char text_buffer[1024];
        float ts = 12 * m_pimpl->m_model->scale;
        float tx = ts / 2;
        float ty = m_pimpl->m_model->height - ts;
        if (SHOW_INFO_TEXT) {
            int hour = m_pimpl->time_of_day() * 24;
            char am_pm = hour < 12 ? 'a' : 'p';
            hour = hour % 12;
            hour = hour ? hour : 12;
            snprintf(
                text_buffer, 1024,
                "(%d, %d) (%.2f, %.2f, %.2f) [%d, %d, %d] %d%cm %dfps",
                m_pimpl->chunked(s->x), m_pimpl->chunked(s->z), s->x, s->y, s->z,
                m_pimpl->m_model->player_count, m_pimpl->m_model->chunk_count,
                face_count * 2, hour, am_pm, fps.fps);
            m_pimpl->render_text(&text_attrib, ALIGN_LEFT, tx, ty, ts, text_buffer);
            ty -= ts * 2;
        }
        if (SHOW_CHAT_TEXT) {
            for (int i = 0; i < MAX_MESSAGES; i++) {
                int index = (m_pimpl->m_model->message_index + i) % MAX_MESSAGES;
                if (strlen(m_pimpl->m_model->messages[index])) {
                    m_pimpl->render_text(&text_attrib, ALIGN_LEFT, tx, ty, ts,
                        m_pimpl->m_model->messages[index]);
                    ty -= ts * 2;
                }
            }
        }
        if (m_pimpl->m_model->typing) {
            snprintf(text_buffer, 1024, "> %s", m_pimpl->m_model->typing_buffer);
            m_pimpl->render_text(&text_attrib, ALIGN_LEFT, tx, ty, ts, text_buffer);
        }
        if (SHOW_PLAYER_NAMES) {
            if (player != me) {
                m_pimpl->render_text(&text_attrib, ALIGN_CENTER, m_pimpl->m_model->width / 2, ts, ts, player->name);
            }
            craft_impl::Player* other = m_pimpl->player_crosshair(player);
            if (other) {
                m_pimpl->render_text(&text_attrib, ALIGN_CENTER, m_pimpl->m_model->width / 2, m_pimpl->m_model->height / 2 - ts - 24, ts, other->name);
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
                m_pimpl->render_text(&text_attrib, ALIGN_CENTER, pw / 2, ts, ts, player->name);
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
    }  // EVENT LOOP

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

    db_save_state(s->x, s->y, s->z, s->rx, s->ry);
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

    SDL_GL_DeleteContext(m_pimpl->m_model->context);
    SDL_DestroyWindow(m_pimpl->m_model->window);
    SDL_Quit();

    return true;
}  // run

// Used by Emscripten mostly to produce a JSON string containing the vertex data
// returns JSON-encoded string:
//  return "{\"name\":\"MyMaze\", \"data\":\"v 1.0 1.0 0.0\\nv -1.0 1.0 0.0\\n...\"}";
std::string craft::get_vertex_data_as_json() const noexcept {
    auto json_writer = [this](const vector<craft_impl::Vertex>& vertices, const vector<craft_impl::Face>& faces) {
        stringstream ss;
        // Set key if outfile is specified
        ss << "{\"name\":\"" << this->m_pimpl->m_gui.outfile << "\", \"data\":[";
        // Wavefront object file header
        ss << "\"# https://www.github.com/zmertens/MazeBuilder\\n\"";
        for (const auto& vertex : vertices) {
            ss << ",\"v " << vertex.x << " " << vertex.y << " " << vertex.z << "\\n\"";
        }
        
        // Note in writing the face that the index is 1-based
        // Also, there is no space after the 'f', until the loop
        for (const auto& face : faces) {
            ss << ",\"f";
            for (auto index : face.vertices) {
                ss << " " << index;
            }
            ss << "\\n\"";
        }

        ss << "]}";

        return ss.str();
    };

    return json_writer(this->m_pimpl->m_vertices, this->m_pimpl->m_faces);
}
