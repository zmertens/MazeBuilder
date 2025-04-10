/// @file craft.h
/// @brief Craft class for the maze builder
/// @details This file contains the Craft class for the maze builder
/// @details Craft engine handles voxel generation and renders to the screen using OpenGL
/// @details Mazes can be generated using Maze Builder
/// @details Supports RESTful-like APIs for web applications by passing voxel data in JSON format
/// @details Interfaces with Emscripten to provide web API support
/// @details Originally written in C99, ported to C++17

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

#include <cstdint>
#include <cstring>
#include <string>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>
#include <utility>
#include <tuple>
#include <list>
#include <vector>

#include <noise/noise.h>

#include "craft_utils.h"
#include "world.h"
#include "cube.h"
#include "db.h"
#include "item.h"
#include "matrix.h"

#include <MazeBuilder/maze_builder.h>

// Movement configurations
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
#define RENDER_SIGN_RADIUS 4
#define DELETE_CHUNK_RADIUS 14
#define COMMIT_INTERVAL 7
#define MAX_CHUNKS 8192
#define MAX_PLAYERS 1
#define NUM_WORKERS 4

#define WORKER_IDLE 0
#define WORKER_BUSY 1
#define WORKER_DONE 2

using namespace mazes;
using namespace std;

struct craft::craft_impl {
    class Gui {
    public:
        bool fullscreen;
        bool vsync;
        bool color_mode_dark;
        bool capture_mouse;
        int chunk_size;
        bool show_items;
        bool show_wireframes;
        bool show_crosshairs;
        bool show_info_text;
        bool apply_bloom_effect;
        float exposure;
        char outfile[64];
        int seed;
        int rows;
        int height;
        int columns;
        int offset_x;
        int offset_z;
        std::string algo;
        int view;
        char tag[MAX_SIGN_LENGTH];

        Gui() : fullscreen(false), vsync(true), color_mode_dark(false),
            capture_mouse(false), chunk_size(8),
            show_items(true), show_wireframes(true), show_crosshairs(true),
            show_info_text(true), apply_bloom_effect(true), exposure(0.39f),
            outfile("my_maze1.obj"), seed(10), rows(25), height(5), columns(18),
            offset_x(0), offset_z(0),
            algo("binary_tree"), view(20), tag("maze here") {

        }

        void reset() {
            for (auto i = 0; i < IM_ARRAYSIZE(outfile); ++i) {
                outfile[i] = '\0';
            }
            outfile[0] = '.';
            outfile[1] = 'o';
            outfile[2] = 'b';
            outfile[3] = 'j';
            rows = 15;
            height = 5;
            columns = 28;
            view = 20;
            algo = "binary_tree";
            seed = 10;
            chunk_size = 8;
            tag[0] = 'H';
            tag[1] = 'i';
            show_crosshairs = true;
            show_info_text = true;
            show_items = true;
            show_wireframes = true;
            capture_mouse = false;
        }
    }; // class

    class BloomTools {
    public:
        GLuint fbo_hdr, fbo_pingpong[2], fbo_final;
        GLuint rbo_bloom_depth;
        // Hold 2 floating point color buffers (1 for normal rendering, 
        //  the other one for brightness treshold values)
        GLuint color_buffers[2], color_buffers_pingpong[2], color_final;

        // State variables
        bool first_iteration, horizontal_blur;
        static constexpr unsigned int NUM_FBO_ITERATIONS = 10;

        BloomTools() : fbo_hdr(0), fbo_pingpong{ 0, 0 }, fbo_final(0)
            , rbo_bloom_depth(0)
            , color_buffers{ 0, 0 }, color_buffers_pingpong{ 0, 0 }, color_final(0)
            , first_iteration(true), horizontal_blur(true) {
        }

        void reset() {
            fbo_hdr = 0;
            rbo_bloom_depth = 0;
            fbo_pingpong[0] = 0;
            fbo_pingpong[1] = 0;
            color_buffers[0] = 0;
            color_buffers[1] = 0;
            color_buffers_pingpong[0] = 0;
            color_buffers_pingpong[1] = 0;
        }

        void gen_framebuffers(int w, int h) {
            glGenFramebuffers(1, &fbo_hdr);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_hdr);

            glGenTextures(2, color_buffers);
            for (auto i = 0; i < 2; ++i) {
                glBindTexture(GL_TEXTURE_2D, color_buffers[i]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, color_buffers[i], 0);
            }

            glGenRenderbuffers(1, &rbo_bloom_depth);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo_bloom_depth);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_bloom_depth);
            // Split color attachments to use for rendering (for this specific framebuffer)
            GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, attachments);

            check_framebuffer();

            // Setup the ping-pong framebuffers for blurring
            glGenFramebuffers(2, fbo_pingpong);
            glGenTextures(2, color_buffers_pingpong);
            for (auto i = 0; i < 2; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_pingpong[i]);
                glBindTexture(GL_TEXTURE_2D, color_buffers_pingpong[i]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_buffers_pingpong[i], 0);
            }

			check_framebuffer();
#if defined(MAZE_DEBUG)
            SDL_Log("Creating FBO with width: %d and height: %d\n", w, h);
#endif

            glGenFramebuffers(1, &fbo_final);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_final);

            glGenTextures(1, &color_final);
            glBindTexture(GL_TEXTURE_2D, color_final);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_final, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        } // gen

        void check_framebuffer() {
            // Check for FBO initialization errors
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                switch (status) {
                case GL_FRAMEBUFFER_UNDEFINED:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "GL_FRAMEBUFFER_UNDEFINED\n");
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n");
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n");
                    break;
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "GL_FRAMEBUFFER_UNSUPPORTED\n");
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\n");
                    break;
#if !defined(__EMSCRIPTEN__)
                case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n");
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n");
                    break;
#endif
                default:
                    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unknown FBO error\n");
                    break;
                }
            }
        } // check_framebuffer
    }; // class

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

    struct WorkerItem {
        int p;
        int q;
        int load;
        Map* block_maps[3][3];
        Map* light_maps[3][3];
        int miny;
        int maxy;
        int faces;
        GLfloat* data;
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
        int id;
        std::string name;
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
        SDL_Window* window;
        SDL_GLContext context;
        std::vector<std::unique_ptr<Worker>> workers;
        Chunk chunks[MAX_CHUNKS];
        int chunk_count;
        int create_radius;
        int render_radius;
        int delete_radius;
        int sign_radius;
        Player player;
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

    // Note: These are public members
    const std::string& m_title;
    const std::string& m_version;
    const int INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT;

    unique_ptr<Model> m_model;
    unique_ptr<Gui> m_gui;

    std::string m_json_data;

    craft_impl(const std::string& title, const std::string& version, int w, int h)
        : m_title(title)
        , m_version(version)
        , INIT_WINDOW_WIDTH(w)
        , INIT_WINDOW_HEIGHT(h)
        , m_model{ make_unique<Model>() }
        , m_gui{ make_unique<Gui>() } {
        this->reset_model();
    }

    int worker_run(void* arg, const mazes::lab& my_mazes) {
        Worker* worker = reinterpret_cast<Worker*>(arg);
        while (1) {
            while (worker->state != WORKER_BUSY && !worker->should_stop) {
                unique_lock<mutex> u_lck(worker->mtx);
                worker->cnd.wait(u_lck);
            }
            if (worker->should_stop) {
                break;
            }
            WorkerItem* worker_item = &worker->item;
            if (worker_item->load) {
                this->load_chunk(worker_item, cref(my_mazes));
            }

            this->compute_chunk(worker_item);

            worker->mtx.lock();
            worker->state = WORKER_DONE;
            worker->mtx.unlock();
        }
        return 0;
    } // worker_run

    void init_worker_threads(const mazes::lab& my_mazes) {
        this->m_model->workers.reserve(NUM_WORKERS);
        for (int i = 0; i < NUM_WORKERS; i++) {
            auto worker = make_unique<Worker>();
            worker->index = i;
            worker->state = WORKER_IDLE;
            worker->thrd = thread([this, &my_mazes](void* arg) { this->worker_run(arg, cref(my_mazes)); }, worker.get());
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

            SDL_Log("Worker thread %d finished!", w->index);
        }
        // Clear the vector after all threads have been joined
        this->m_model->workers.clear();
    }

    void del_buffer(GLuint buffer) const {
        glDeleteBuffers(1, &buffer);
    }

    GLuint gen_buffer(GLsizei size, GLfloat* data) const {
        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return buffer;
    }

    GLfloat* malloc_faces(std::size_t components, std::size_t faces) const {
        return (GLfloat*)SDL_malloc(sizeof(GLfloat) * 6 * components * faces);
    }

    /**
     * Generate a buffer for faces - data is not freed here
     */
    GLuint gen_faces(GLsizei components, GLsizei faces, GLfloat* data) const {
        GLuint buffer = this->gen_buffer(sizeof(GLfloat) * 6 * components * faces, data);
        return buffer;
    }

    int chunked(float x) const {
        return static_cast<int>(SDL_floorf(SDL_roundf(x) / static_cast<float>(this->m_gui->chunk_size)));
    }

    double get_time() const {
        return (static_cast<double>(SDL_GetTicks()) + static_cast<double>(this->m_model->start_time) - static_cast<double>(this->m_model->start_ticks)) / 1000.0;
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
        } else {
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
        return result;
    }

    void get_sight_vector(float rx, float ry, float* vx, float* vy, float* vz) const {
        float m = SDL_cosf(ry);
        *vx = SDL_cosf(rx - static_cast<float>(RADIANS(90))) * m;
        *vy = SDL_sinf(ry);
        *vz = SDL_sinf(rx - static_cast<float>(RADIANS(90))) * m;
    }

    void get_motion_vector(int flying, int sz, int sx, float rx, float ry,
        float* vx, float* vy, float* vz) const {
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
        } else {
            *vx = SDL_cosf(rx + strafe);
            *vy = 0;
            *vz = SDL_sinf(rx + strafe);
        }
    }

    GLuint gen_crosshair_buffer() {
        float x = static_cast<float>(this->m_model->voxel_scene_w) / 2.0f;
        float y = static_cast<float>(this->m_model->voxel_scene_h) / 2.0f;
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

    GLuint gen_cube_buffer(float x, float y, float z, float n, int w) {
        GLfloat* data = malloc_faces(10, 6);
        float ao[6][4] = { 0 };
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
        GLfloat* data = malloc_faces(10, 4);
        float ao = 0;
        float light = 1;
        make_plant(data, ao, light, x, y, z, n, w, 45);
        return gen_faces(10, 4, data);
    }

    GLuint gen_player_buffer(float x, float y, float z, float rx, float ry) {
        GLfloat* data = malloc_faces(10, 6);
        make_player(data, x, y, z, rx, ry);
        return gen_faces(10, 6, data);
    }

    GLuint gen_text_buffer(float x, float y, float n, char* text) {
        GLsizei length = static_cast<GLsizei>(SDL_strlen(text));
        GLfloat* data = malloc_faces(4, length);
        for (int i = 0; i < length; i++) {
            make_character(data + i * 24, x, y, n / 2, n, text[i]);
            x += n;
        }
        return gen_faces(4, length, data);
    }

    void draw_triangles_3d_ao(Attrib* attrib, GLuint buffer, int count) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glEnableVertexAttribArray(attrib->position);
        glEnableVertexAttribArray(attrib->normal);
        glEnableVertexAttribArray(attrib->uv);
        glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 10, 0);
        glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 10, (GLvoid*)(sizeof(GLfloat) * 3));
        glVertexAttribPointer(attrib->uv, 4, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 10, (GLvoid*)(sizeof(GLfloat) * 6));
        glDrawArrays(GL_TRIANGLES, 0, count);
        glDisableVertexAttribArray(attrib->position);
        glDisableVertexAttribArray(attrib->normal);
        glDisableVertexAttribArray(attrib->uv);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_triangles_3d_text(Attrib* attrib, GLuint buffer, int count) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glEnableVertexAttribArray(attrib->position);
        glEnableVertexAttribArray(attrib->uv);
        glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 5, 0);
        glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 5, (GLvoid*)(sizeof(GLfloat) * 3));
        glDrawArrays(GL_TRIANGLES, 0, count);
        glDisableVertexAttribArray(attrib->position);
        glDisableVertexAttribArray(attrib->uv);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_triangles_3d(Attrib* attrib, GLuint buffer, int count) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);

        glEnableVertexAttribArray(attrib->position);

        glEnableVertexAttribArray(attrib->normal);
        glEnableVertexAttribArray(attrib->uv);

        glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, 0);
        glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (GLvoid*)(sizeof(GLfloat) * 3));
        glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (GLvoid*)(sizeof(GLfloat) * 6));

        glDrawArrays(GL_TRIANGLES, 0, count);

        glDisableVertexAttribArray(attrib->position);
        glDisableVertexAttribArray(attrib->normal);
        glDisableVertexAttribArray(attrib->uv);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_triangles_2d(Attrib* attrib, GLuint buffer, GLsizei count) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glEnableVertexAttribArray(attrib->position);
        glEnableVertexAttribArray(attrib->uv);
        glVertexAttribPointer(attrib->position, 2, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 4, 0);
        glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
            sizeof(GLfloat) * 4, (GLvoid*)(sizeof(GLfloat) * 2));
        glDrawArrays(GL_TRIANGLES, 0, count);
        glDisableVertexAttribArray(attrib->position);
        glDisableVertexAttribArray(attrib->uv);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_lines(Attrib* attrib, GLuint buffer, int components, int count) {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glEnableVertexAttribArray(attrib->position);
        glVertexAttribPointer(
            attrib->position, components, GL_FLOAT, GL_FALSE, 0, 0);
        glDrawArrays(GL_LINES, 0, count);
        glDisableVertexAttribArray(attrib->position);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw_chunk(Attrib* attrib, Chunk* chunk) {
        draw_triangles_3d_ao(attrib, chunk->buffer, chunk->faces * 6);
    }

    void draw_item(Attrib* attrib, GLuint buffer, int count) {
        draw_triangles_3d_ao(attrib, buffer, count);
    }

    void draw_text(Attrib* attrib, GLuint buffer, GLsizei length) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        draw_triangles_2d(attrib, buffer, length * 6);
        glDisable(GL_BLEND);
    }

    void draw_signs(Attrib* attrib, Chunk* chunk) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-8, -1024);
        draw_triangles_3d_text(attrib, chunk->sign_buffer, chunk->sign_faces * 6);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    void draw_sign(Attrib* attrib, GLuint buffer, int length) {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-8, -1024);
        draw_triangles_3d_text(attrib, buffer, length * 6);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    void draw_cube(Attrib* attrib, GLuint buffer) {
        draw_item(attrib, buffer, 36);
    }

    void draw_plant(Attrib* attrib, GLuint buffer) {
        draw_item(attrib, buffer, 24);
    }

    void draw_player(Attrib* attrib, Player* player) {
        draw_cube(attrib, player->buffer);
    }

    Player* find_player(int id) {
        for (int i = 0; i < this->m_model->player_count; i++) {
            Player* player = &this->m_model->player;
            if (player->id == id) {
                return player;
            }
        }
        return 0;
    }

    void delete_all_players() {
        for (int i = 0; i < this->m_model->player_count; i++) {
            Player* player = &this->m_model->player;
            this->del_buffer(player->buffer);
        }
        this->m_model->player_count = 0;
    }

    Chunk* find_chunk(int p, int q) const {
        for (int i = 0; i < this->m_model->chunk_count; i++) {
            Chunk* chunk = this->m_model->chunks + i;
            if (chunk->p == p && chunk->q == q) {
                return chunk;
            }
        }
        return 0;
    }

    int chunk_distance(Chunk* chunk, int p, int q) {
        int dp = SDL_abs(chunk->p - p);
        int dq = SDL_abs(chunk->q - q);
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
                } else {
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
        Chunk* chunk = this->find_chunk(p, q);
        if (chunk) {
            Map* map = &chunk->map;
            MAP_FOR_EACH(map, ex, ey, ez, ew) {
                // item.h -> is_obstacle
                if (is_obstacle(ew) && ex == nx && ez == nz) {
                    result = SDL_max(result, ey);
                }
            } END_MAP_FOR_EACH;
        }
        return result;
    }

    int _hit_test(Map* map, float max_distance, int previous, float x, float y, float z, float vx, float vy, float vz, int* hx, int* hy, int* hz) {
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
                    } else {
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

    int hit_test(int previous, float x, float y, float z, float rx, float ry, int* bx, int* by, int* bz) {
        int result = 0;
        float best = 0;
        int p = this->chunked(x);
        int q = this->chunked(z);
        float vx, vy, vz;
        this->get_sight_vector(rx, ry, &vx, &vy, &vz);
        for (int i = 0; i < this->m_model->chunk_count; i++) {
            Chunk* chunk = this->m_model->chunks + i;
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

    /**
     * @brief Check if selected block is colliding with player wireframe
     */
    int hit_test_face(Player* player, int* x, int* y, int* z, int* face) {
        State* s = &player->state;
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

    /**
     * @brief Check if the player is colliding with the map
     *
     */
    int collide(int height, float* x, float* y, float* z) const {
        int result = 0;
        int p = this->chunked(*x);
        int q = this->chunked(*z);
        Chunk* chunk = this->find_chunk(p, q);
        if (!chunk) {
            SDL_Log("Could find chunk: %d %d", p, q);
            return result;
        }
        Map* map = &chunk->map;
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

    int _gen_sign_buffer(GLfloat* data, float x, float y, float z, int face, const char* text) const {
        static constexpr int glyph_dx[8] = { 0, 0, -1, 1, 1, 0, -1, 0 };
        static constexpr int glyph_dz[8] = { 1, -1, 0, 0, 0, -1, 0, 1 };
        static constexpr int line_dx[8] = { 0, 0, 0, 0, 0, 1, 0, -1 };
        static constexpr int line_dy[8] = { -1, -1, -1, -1, 0, 0, 0, 0 };
        static constexpr int line_dz[8] = { 0, 0, 0, 0, 1, 0, -1, 0 };
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
        char* key;
        // util.h -> tokenize
        char* line = tokenize(lines, "\n", &key);
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

    void gen_sign_buffer(Chunk* chunk) const {
        SignList* signs = &chunk->signs;

        // first pass - count characters
        size_t max_faces = 0;
        for (int i = 0; i < signs->size; i++) {
            Sign* e = signs->data + i;
            max_faces += SDL_strlen(e->text);
        }

        // second pass - generate geometry
        GLfloat* data = malloc_faces(5, max_faces);
        size_t faces = 0;
        for (int i = 0; i < signs->size; i++) {
            Sign* e = signs->data + i;
            faces += static_cast<size_t>(this->_gen_sign_buffer(data + static_cast<int>(faces) * 30,
                static_cast<float>(e->x),
                static_cast<float>(e->y),
                static_cast<float>(e->z), e->face, e->text));
        }

        this->del_buffer(chunk->sign_buffer);
        chunk->sign_buffer = this->gen_faces(5, static_cast<GLsizei>(faces), data);
        chunk->sign_faces = static_cast<int>(faces);
    }

    int has_lights(Chunk* chunk) const {
        for (int dp = -1; dp <= 1; dp++) {
            for (int dq = -1; dq <= 1; dq++) {
                Chunk* other = chunk;
                if (dp || dq) {
                    other = this->find_chunk(chunk->p + dp, chunk->q + dq);
                }
                if (!other) {
                    continue;
                }
                Map* map = &other->lights;
                if (map->size) {
                    return 1;
                }
            }
        }
        return 0;
    }

    void dirty_chunk(Chunk* chunk) const {
        chunk->dirty = 1;
        if (has_lights(chunk)) {
            for (int dp = -1; dp <= 1; dp++) {
                for (int dq = -1; dq <= 1; dq++) {
                    Chunk* other = this->find_chunk(chunk->p + dp, chunk->q + dq);
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
        static constexpr float curve[4] = { 0.0, 0.25, 0.5, 0.75 };
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

    void light_fill(char* opaque, char* light, int x, int y, int z, int w, int force) const {
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
    void compute_chunk(WorkerItem* item) const {
        char* opaque = (char*)SDL_calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
        char* light = (char*)SDL_calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
        char* highest = (char*)SDL_calloc(XZ_SIZE * XZ_SIZE, sizeof(char));

        int ox = item->p * this->m_gui->chunk_size - this->m_gui->chunk_size - 1;
        int oy = -1;
        int oz = item->q * this->m_gui->chunk_size - this->m_gui->chunk_size - 1;

        // check for lights
        int has_light = 0;
        for (int a = 0; a < 3; a++) {
            for (int b = 0; b < 3; b++) {
                Map* map = item->light_maps[a][b];
                if (map && map->size) {
                    has_light = 1;
                }
            }
        }

        // populate opaque array
        for (int a = 0; a < 3; a++) {
            for (int b = 0; b < 3; b++) {
                Map* block_map = item->block_maps[a][b];
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
                    Map* map = item->light_maps[a][b];
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

        Map* block_map = item->block_maps[1][1];

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
        GLfloat* data = malloc_faces(components, faces);
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
            char neighbors[27] = { 0 };
            char lights[27] = { 0 };
            float shades[27] = { 0 };
            int index = 0;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dz = -1; dz <= 1; dz++) {
                        neighbors[index] = opaque[XYZ(x + dx, y + dy, z + dz)];
                        lights[index] = light[XYZ(x + dx, y + dy, z + dz)];
                        shades[index] = 0;
                        int highest_index = XZ(x + dx, z + dz);
                        if (highest_index < XZ_SIZE * XZ_SIZE && y + dy <= highest[highest_index]) {
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
            } else {
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

    void generate_chunk(Chunk* chunk, WorkerItem* item) const {
        chunk->miny = item->miny;
        chunk->maxy = item->maxy;
        chunk->faces = item->faces;
        this->del_buffer(chunk->buffer);
        chunk->buffer = this->gen_faces(10, item->faces, item->data);
        this->gen_sign_buffer(chunk);
    }

    void gen_chunk_buffer(Chunk* chunk) const {
        WorkerItem _item;
        WorkerItem* item = &_item;
        item->p = chunk->p;
        item->q = chunk->q;
        for (int dp = -1; dp <= 1; dp++) {
            for (int dq = -1; dq <= 1; dq++) {
                Chunk* other = chunk;
                if (dp || dq) {
                    other = this->find_chunk(chunk->p + dp, chunk->q + dq);
                }
                if (other) {
                    item->block_maps[dp + 1][dq + 1] = &other->map;
                    item->light_maps[dp + 1][dq + 1] = &other->lights;
                } else {
                    item->block_maps[dp + 1][dq + 1] = 0;
                    item->light_maps[dp + 1][dq + 1] = 0;
                }
            }
        }
        this->compute_chunk(item);
        this->generate_chunk(chunk, item);
        chunk->dirty = 0;
    }

    static void map_set_func(int x, int y, int z, int w, Map* m) {
        map_set(m, x, y, z, w);
    }

    // Create a chunk that represents a unique portion of the world
    // p, q represents the chunk key
    void load_chunk(WorkerItem* item, const mazes::lab& my_mazes) {
        int p = item->p;
        int q = item->q;

        Map* block_map = item->block_maps[1][1];
        Map* light_map = item->light_maps[1][1];
        // world.h
        static world my_world;
        my_world.create_world(p, q, map_set_func, block_map, this->m_gui->chunk_size, cref(my_mazes));
        db_load_blocks(block_map, p, q);
        db_load_lights(light_map, p, q);
    }

    /**
    * @brief called by ensure_chunk_workers, create_chunk
    * @return
    */
    void init_chunk(Chunk* chunk, int p, int q) {
        chunk->p = p;
        chunk->q = q;
        chunk->faces = 0;
        chunk->sign_faces = 0;
        chunk->buffer = 0;
        chunk->sign_buffer = 0;
        dirty_chunk(chunk);
        SignList* signs = &chunk->signs;
        sign_list_alloc(signs, 16);
        db_load_signs(signs, p, q);
        Map* block_map = &chunk->map;
        Map* light_map = &chunk->lights;
        int dx = p * this->m_gui->chunk_size - 1;
        int dy = 0;
        int dz = q * this->m_gui->chunk_size - 1;
        map_alloc(block_map, dx, dy, dz, 0x7fff);
        map_alloc(light_map, dx, dy, dz, 0xf);
    }

    void create_chunk(Chunk* chunk, int p, int q, const mazes::lab& my_mazes) {
        init_chunk(chunk, p, q);

        WorkerItem _item;
        WorkerItem* item = &_item;
        item->p = chunk->p;
        item->q = chunk->q;
        item->block_maps[1][1] = &chunk->map;
        item->light_maps[1][1] = &chunk->lights;

        load_chunk(item, cref(my_mazes));
    }

    void delete_chunks() {
        int count = this->m_model->chunk_count;
        State* s1 = &this->m_model->player.state;
        for (int i = 0; i < count; i++) {
            Chunk* chunk = this->m_model->chunks + i;
            int remove_chunk = 1;
            int p = chunked(s1->x);
            int q = chunked(s1->z);
            if (chunk_distance(chunk, p, q) < this->m_model->delete_radius) {
                remove_chunk = 0;
                break;
            }
            if (remove_chunk) {
                map_free(&chunk->map);
                map_free(&chunk->lights);
                sign_list_free(&chunk->signs);
                del_buffer(chunk->buffer);
                del_buffer(chunk->sign_buffer);
                Chunk* other = this->m_model->chunks + (--count);
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
            Chunk* chunk = this->m_model->chunks + i;
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
                WorkerItem* item = &worker->item;
                Chunk* chunk = find_chunk(item->p, item->q);
                if (chunk) {
                    if (item->load) {
                        Map* block_map = item->block_maps[1][1];
                        Map* light_map = item->light_maps[1][1];
                        map_free(&chunk->map);
                        map_free(&chunk->lights);
                        map_copy(&chunk->map, block_map);
                        map_copy(&chunk->lights, light_map);
                    }
                    generate_chunk(chunk, item);
                }
                for (int a = 0; a < 3; a++) {
                    for (int b = 0; b < 3; b++) {
                        Map* block_map = item->block_maps[a][b];
                        Map* light_map = item->light_maps[a][b];
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

    // Used to init the terrain (chunks) around the player
    void force_chunks(Player* player, const mazes::lab& my_mazes) {
        State* s = &player->state;
        int p = chunked(s->x);
        int q = chunked(s->z);

        int r = 1;
        for (int dp = -r; dp <= r; dp++) {
            for (int dq = -r; dq <= r; dq++) {
                int a = p + dp;
                int b = q + dq;
                Chunk* chunk = find_chunk(a, b);
                if (chunk) {
                    if (chunk->dirty) {
                        gen_chunk_buffer(chunk);
                    }
                } else if (this->m_model->chunk_count < MAX_CHUNKS) {
                    chunk = this->m_model->chunks + this->m_model->chunk_count++;
                    create_chunk(chunk, a, b, cref(my_mazes));
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
    void ensure_chunks_worker(Player* player, Worker* worker) {
        State* s = &player->state;
        float matrix[16];
        set_matrix_3d(matrix, this->m_model->voxel_scene_w, this->m_model->voxel_scene_h, s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        float planes[6][4];
        frustum_planes(planes, this->m_model->render_radius, matrix);
        int p = chunked(s->x);
        int q = chunked(s->z);
        // int r = this->m_model->create_radius;
        int r{ this->m_model->create_radius };
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
                Chunk* chunk = find_chunk(a, b);
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
        Chunk* chunk = find_chunk(a, b);
        // Check if the chunk is already loaded
        if (!chunk) {
            load = 1;
            if (this->m_model->chunk_count < MAX_CHUNKS) {
                chunk = this->m_model->chunks + this->m_model->chunk_count++;
                init_chunk(chunk, a, b);
            } else {
                return;
            }
        }
        WorkerItem* item = &worker->item;
        item->p = chunk->p;
        item->q = chunk->q;
        item->load = load;
        for (int dp = -1; dp <= 1; dp++) {
            for (int dq = -1; dq <= 1; dq++) {
                Chunk* other = chunk;
                if (dp || dq) {
                    other = find_chunk(chunk->p + dp, chunk->q + dq);
                }
                if (other) {
                    // These maps are freed using C-library free function
                    Map* block_map = (Map*)malloc(sizeof(Map));
                    map_copy(block_map, &other->map);
                    Map* light_map = (Map*)malloc(sizeof(Map));
                    map_copy(light_map, &other->lights);
                    item->block_maps[dp + 1][dq + 1] = block_map;
                    item->light_maps[dp + 1][dq + 1] = light_map;
                } else {
                    item->block_maps[dp + 1][dq + 1] = 0;
                    item->light_maps[dp + 1][dq + 1] = 0;
                }
            }
        }
        chunk->dirty = 0;
        worker->state = WORKER_BUSY;
        worker->cnd.notify_one();
    } // ensure chunks worker

    void ensure_chunks(Player* player, const mazes::lab& my_mazes) {
        check_workers();
        force_chunks(player, cref(my_mazes));
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
        Chunk* chunk = find_chunk(p, q);
        if (chunk) {
            SignList* signs = &chunk->signs;
            if (sign_list_remove_all(signs, x, y, z)) {
                chunk->dirty = 1;
                db_delete_signs(x, y, z);
            }
        } else {
            db_delete_signs(x, y, z);
        }
    }

    void unset_sign_face(int x, int y, int z, int face) const {
        int p = chunked(static_cast<float>(x));
        int q = chunked(static_cast<float>(z));
        Chunk* chunk = find_chunk(p, q);
        if (chunk) {
            SignList* signs = &chunk->signs;
            if (sign_list_remove(signs, x, y, z, face)) {
                chunk->dirty = 1;
                db_delete_sign(x, y, z, face);
            }
        } else {
            db_delete_sign(x, y, z, face);
        }
    }

    void _set_sign(int p, int q, int x, int y, int z, int face, const char* text, int dirty) const {
        if (SDL_strlen(text) == 0) {
            unset_sign_face(x, y, z, face);
            return;
        }
        Chunk* chunk = find_chunk(p, q);
        if (chunk) {
            SignList* signs = &chunk->signs;
            sign_list_add(signs, x, y, z, face, text);
            if (dirty) {
                chunk->dirty = 1;
            }
        }
        db_insert_sign(p, q, x, y, z, face, text);
    }

    void set_sign(int x, int y, int z, int face, const char* text) const {
        int p = chunked(static_cast<float>(x));
        int q = chunked(static_cast<float>(z));
        _set_sign(p, q, x, y, z, face, text, 1);
    }

    void toggle_light(int x, int y, int z) const {
        int p = chunked(static_cast<float>(x));
        int q = chunked(static_cast<float>(z));
        Chunk* chunk = find_chunk(p, q);
        if (chunk) {
            Map* map = &chunk->lights;
            int w = map_get(map, x, y, z) ? 0 : 15;
            map_set(map, x, y, z, w);
            db_insert_light(p, q, x, y, z, w);
            dirty_chunk(chunk);
        }
    }

    void set_light(int p, int q, int x, int y, int z, int w) const {
        Chunk* chunk = find_chunk(p, q);
        if (chunk) {
            Map* map = &chunk->lights;
            if (map_set(map, x, y, z, w)) {
                dirty_chunk(chunk);
                db_insert_light(p, q, x, y, z, w);
            }
        } else {
            db_insert_light(p, q, x, y, z, w);
        }
    }

    void _set_block(int p, int q, int x, int y, int z, int w, int dirty) const {
        Chunk* chunk = find_chunk(p, q);
        if (chunk) {
            Map* map = &chunk->map;
            if (map_set(map, x, y, z, w)) {
                if (dirty) {
                    dirty_chunk(chunk);
                }
                db_insert_block(p, q, x, y, z, w);
            }
        } else {
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
        Chunk* chunk = find_chunk(p, q);
        if (chunk) {
            Map* map = &chunk->map;
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
    int render_chunks(Attrib* attrib, Player* player, GLuint texture, const mazes::lab& my_mazes) {
        int result = 0;
        State* s = &player->state;
        this->ensure_chunks(player, cref(my_mazes));
        int p = this->chunked(s->x);
        int q = this->chunked(s->z);
        float light = this->get_daylight();
        float matrix[16];
        // matrix.cpp -> set_matrix_3d
        set_matrix_3d(
            matrix, this->m_model->voxel_scene_w, this->m_model->voxel_scene_h,
            s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        float planes[6][4];
        // matrix.cpp -> frustum_planes
        frustum_planes(planes, this->m_model->render_radius, matrix);
        glUseProgram(attrib->program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform3f(attrib->camera, s->x, s->y, s->z);
        glUniform1i(attrib->sampler, 0);
        glUniform1f(attrib->extra2, light);
        glUniform1f(attrib->extra3, static_cast<GLfloat>(this->m_model->render_radius * this->m_gui->chunk_size));
        glUniform1i(attrib->extra4, static_cast<int>(this->m_model->is_ortho));
        glUniform1f(attrib->timer, this->time_of_day());
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        for (int i = 0; i < this->m_model->chunk_count; i++) {
            Chunk* chunk = this->m_model->chunks + i;
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

    void render_signs(Attrib* attrib, Player* player, GLuint sign) {
        State* s = &player->state;
        int p = chunked(s->x);
        int q = chunked(s->z);
        float matrix[16];
        set_matrix_3d(
            matrix, this->m_model->voxel_scene_w, this->m_model->voxel_scene_h,
            s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        float planes[6][4];
        frustum_planes(planes, this->m_model->render_radius, matrix);

        glUseProgram(attrib->program);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, sign);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        glUniform1i(attrib->sampler, 2);
        glUniform1i(attrib->extra1, 1);

        for (int i = 0; i < this->m_model->chunk_count; i++) {
            Chunk* chunk = this->m_model->chunks + i;
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

    void render_sign(Attrib* attrib, Player* player, GLuint sign) {
        int x, y, z, face;
        if (!hit_test_face(player, &x, &y, &z, &face)) {
            return;
        }

        State* s = &player->state;
        float matrix[16];
        set_matrix_3d(
            matrix, this->m_model->voxel_scene_w, this->m_model->voxel_scene_h,
            s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        glUseProgram(attrib->program);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, sign);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        glUniform1i(attrib->sampler, 2);
        glUniform1i(attrib->extra1, 1);
        char text[MAX_SIGN_LENGTH];
        SDL_strlcpy(text, this->m_gui->tag, MAX_SIGN_LENGTH);
        text[MAX_SIGN_LENGTH - 1] = '\0';
        GLfloat* data = malloc_faces(5, SDL_strlen(text));
        int length = _gen_sign_buffer(data, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), face, text);
        GLuint buffer = gen_faces(5, length, data);
        draw_sign(attrib, buffer, length);
        del_buffer(buffer);
    }

    void render_players(Attrib* attrib, Player* player) {
        State* s = &player->state;
        float matrix[16];
        set_matrix_3d(
            matrix, this->m_model->voxel_scene_w, this->m_model->voxel_scene_h,
            s->x, s->y, s->z, s->rx, s->ry, this->m_model->fov, static_cast<int>(this->m_model->is_ortho), this->m_model->render_radius);
        glUseProgram(attrib->program);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        glUniform3f(attrib->camera, s->x, s->y, s->z);
        glUniform1i(attrib->sampler, 0);
        glUniform1f(attrib->timer, time_of_day());
        for (int i = 0; i < this->m_model->player_count; i++) {
            Player* other = &this->m_model->player;
            if (other != player) {
                draw_player(attrib, other);
            }
        }
    }

    void render_wireframe(Attrib* attrib, Player* player) {
        State* s = &player->state;
        float matrix[16];
        set_matrix_3d(
            matrix, this->m_model->voxel_scene_w, this->m_model->voxel_scene_h,
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

    void render_crosshairs(Attrib* attrib) {
        float matrix[16];
        set_matrix_2d(matrix, this->m_model->voxel_scene_w, this->m_model->voxel_scene_h);
        glUseProgram(attrib->program);
        glLineWidth(static_cast<GLfloat>(4 * this->m_model->scale));
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        GLuint crosshair_buffer = gen_crosshair_buffer();
        draw_lines(attrib, crosshair_buffer, 2, 4);
        del_buffer(crosshair_buffer);
    }

    void render_item(Attrib* attrib, GLuint texture) {
        float matrix[16];
        set_matrix_item(matrix, this->m_model->voxel_scene_w, this->m_model->voxel_scene_h, this->m_model->scale);
        glUseProgram(attrib->program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        glUniform3f(attrib->camera, 0, 0, 5);
        glUniform1i(attrib->sampler, 0);
        glUniform1f(attrib->timer, time_of_day());
        int w = items[this->m_model->item_index];
        if (is_plant(w)) {
            GLuint buffer = gen_plant_buffer(0, 0, 0, 0.5, w);
            draw_plant(attrib, buffer);
            del_buffer(buffer);
        } else {
            GLuint buffer = gen_cube_buffer(0, 0, 0, 0.5, w);
            draw_cube(attrib, buffer);
            del_buffer(buffer);
        }
    }

    void render_text(Attrib* attrib, GLuint font, int justify, float x, float y, float n, char* text) {
        float matrix[16];
        set_matrix_2d(matrix, this->m_model->voxel_scene_w, this->m_model->voxel_scene_h);
        glUseProgram(attrib->program);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        glUniform1i(attrib->sampler, 3);
        glUniform1i(attrib->extra1, 0);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, font);
        GLsizei length = static_cast<GLsizei>(SDL_strlen(text));
        x -= n * justify * (length - 1) / 2;
        GLuint buffer = gen_text_buffer(x, y, n, text);
        draw_text(attrib, buffer, length);
        del_buffer(buffer);
    }

    void on_light() {
        State* s = &this->m_model->player.state;
        int hx, hy, hz;
        int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        if (hy > 0 && hy < 256 && is_destructable(hw)) {
            toggle_light(hx, hy, hz);
        }
    }

    void on_left_click() {
        State* s = &this->m_model->player.state;
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
        State* s = &this->m_model->player.state;
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
        State* s = &this->m_model->player.state;
        int hx, hy, hz;
        int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        for (int i = 0; i < item_count; i++) {
            if (items[i] == hw) {
                this->m_model->item_index = i;
#if defined(MAZE_DEBUG)
                SDL_Log("Copying item index: %d\n", i);
#endif
                break;
            }
        }
    }

    /**
    * Reference: https://github.com/rswinkle/Craft/blob/sdl/src/main.c
    * @brief Handle SDL events and motion
    * @param dt
    * @return bool return true when events are handled successfully or not
    */
    bool handle_events_and_motion(double dt, bool& window_resizes) {
        static float dy = 0;
        State* s = &this->m_model->player.state;
        int sz = 0;
        int sx = 0;
        float mouse_mv = SDL_min(0.0025, dt);
        float dir_mv = 0.025f;
        int sc = -1;

        SDL_Keymod mod_state = SDL_GetModState();

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL3_ProcessEvent(&e);

            switch (e.type) {
            case SDL_EVENT_QUIT: return false;
            case SDL_EVENT_KEY_DOWN: {
                sc = e.key.scancode;
                switch (sc) {
                case SDL_SCANCODE_ESCAPE: {
                    SDL_SetWindowRelativeMouseMode(this->m_model->window, false);
                    this->m_gui->capture_mouse = false;
                    this->m_gui->fullscreen = false;
                    break;
                }
                case SDL_SCANCODE_RETURN: {
                    if (mod_state) {
                        this->on_right_click();
                    } else {
                        this->on_left_click();
                    }
                    break;
                }
                // Change block type when mouse is captured
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
                    if (this->m_gui->capture_mouse) {
                        this->m_model->item_index = (sc - SDL_SCANCODE_1);
                    }
                    break;
                }
                case KEY_FLY: {
                    if (this->m_gui->capture_mouse) {
                        this->m_model->flying = !this->m_model->flying;
                    }
                    break;
                }
                case KEY_ITEM_NEXT: {
                    if (this->m_gui->capture_mouse) {
                        this->m_model->item_index = (this->m_model->item_index + 1) % item_count; 
                    }
                    break;
                }
                case KEY_ITEM_PREV: {
                    if (this->m_gui->capture_mouse) {
                        this->m_model->item_index--;
                        if (this->m_model->item_index < 0) {
                            this->m_model->item_index = item_count - 1;
                        }
                    }
                    break;
                }
                case KEY_TAG: {
#if defined(MAZE_DEBUG)
                    SDL_Log("Tag: %s\n", this->m_gui->tag);
#endif
                    int x, y, z, face;
                    if (this->m_gui->capture_mouse && hit_test_face(&this->m_model->player, &x, &y, &z, &face)) {
                        set_sign(x, y, z, face, this->m_gui->tag);
                    } else if (!this->m_gui->capture_mouse) {
                        SDL_StartTextInput(this->m_model->window);
                    }
                    break;
                }
                } // switch
                break;
            } // case SDL_EVENT_KEY_DOWN
            case SDL_EVENT_TEXT_INPUT: {
                if (!this->m_gui->capture_mouse) {
                    if (SDL_strlen(this->m_gui->tag) < MAX_SIGN_LENGTH) {
                        SDL_strlcat(this->m_gui->tag, e.text.text, MAX_SIGN_LENGTH);
                        SDL_StopTextInput(this->m_model->window);
                    }
                }
                break;
            }
            case SDL_EVENT_FINGER_MOTION:
            case SDL_EVENT_MOUSE_MOTION: {
                if (this->m_gui->capture_mouse) {
                    // Adjust mouse motion based on relative center of voxel_scene_size
                    float adjusted_xrel = e.motion.xrel - static_cast<float>(m_model->voxel_scene_w) / 2.f;
                    float adjusted_yrel = e.motion.yrel - static_cast<float>(m_model->voxel_scene_h) / 2.f;

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
            case SDL_EVENT_FINGER_UP:
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                if (this->m_gui->capture_mouse) {
                    if (e.button.button == SDL_BUTTON_LEFT) {
                        if (mod_state) {
                            on_right_click();
                        } else {
                            on_left_click();
                        }
                    } else if (e.button.button == SDL_BUTTON_RIGHT) {
                        if (mod_state) {
                            on_light();
                        } else {
                            on_right_click();
                        }
                    } else if (e.button.button == SDL_BUTTON_MIDDLE) {
                        on_middle_click();
                    }
                }

                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                if (this->m_gui->capture_mouse) {
                    if (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL) this->m_model->item_index += e.wheel.y;
                    else this->m_model->item_index -= e.wheel.y;

                    if (this->m_model->item_index < 0) this->m_model->item_index = item_count - 1;
                    else this->m_model->item_index %= item_count;
                }
                break;
            }
            case SDL_EVENT_WINDOW_EXPOSED:
            case SDL_EVENT_WINDOW_RESIZED: {
                window_resizes = true;
                this->m_model->scale = get_scale_factor();
                break;
            }
            } // switch
        } // SDL_Event

        // Handle motion updates
        const bool* state = SDL_GetKeyboardState(nullptr);

        if (this->m_gui->capture_mouse) {
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


            float vx, vy, vz;
            get_motion_vector(this->m_model->flying, sz, sx, s->rx, s->ry, &vx, &vy, &vz);
            if (state[KEY_JUMP]) {
                if (this->m_model->flying) {
                    vy = 1;
                } else if (dy == 0) {
                    dy = 8;
                }
            }

            float speed = this->m_model->flying ? 16 : 5;
            float estimate = SDL_roundf(SDL_sqrtf(
                SDL_powf(vx * speed, 2.f) +
                SDL_powf(vy * speed + dy, 2.f) +
                SDL_powf(vz * speed, 2.f)) * 8.f);
            float step = SDL_max(dt, estimate);
            float ut = dt / step;
            vx = vx * ut * speed;
            vy = vy * ut * speed;
            vz = vz * ut * speed;
            for (int i = 0; i < static_cast<int>(step); i++) {
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
        } // capture_mouse

        return true;
    } // handle_events_and_motion

    /**
     * @brief Check what fullscreen modes are avaialble
     */
    void check_fullscreen_modes() {
        SDL_DisplayID display = SDL_GetPrimaryDisplay();
        int num_modes = 0;
        const SDL_DisplayMode* const* modes = SDL_GetFullscreenDisplayModes(display, &num_modes);
        if (modes) {
            for (int i = 0; i < num_modes; ++i) {
                const SDL_DisplayMode* mode = modes[i];
                SDL_Log("Display %" SDL_PRIu32 " mode %d: %dx%d@%gx %gHz\n",
                    display, i, mode->w, mode->h, mode->pixel_density, mode->refresh_rate);
            }
        }
    }

    /**
     * @brief Create SDL/GL window and context, check display modes
     */
    void create_window_and_context() {
#if defined(MAZE_DEBUG)
        SDL_Log("Setting SDL_GL_CONTEXT_DEBUG_FLAG\n");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
#endif

#if defined(__EMSCRIPTEN__)
        SDL_Log("Setting SDL_GL_CONTEXT_PROFILE_ES\n");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
        SDL_Log("Setting SDL_GL_CONTEXT_PROFILE_CORE\n");
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
        char title_formatted[32];
        SDL_snprintf(title_formatted, 32, "%s - %s\n", this->m_title.c_str(), this->m_version.c_str());
        this->m_model->window = SDL_CreateWindow(title_formatted, INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT, window_flags);
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
        SDL_Surface* icon_surface = SDL_LoadBMP_IO(SDL_IOFromFile(icon_path, "rb"), true);
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
        this->m_model->create_radius = CREATE_CHUNK_RADIUS;
        this->m_model->render_radius = RENDER_CHUNK_RADIUS;
        this->m_model->delete_radius = DELETE_CHUNK_RADIUS;
        this->m_model->sign_radius = RENDER_SIGN_RADIUS;
        SDL_memset(reinterpret_cast<void*>(&this->m_model->player), 0, sizeof(Player) * MAX_PLAYERS);
        this->m_model->player.state.y = 64;
        this->m_model->player_count = 1;
        this->m_model->flying = false;
        this->m_model->item_index = 0;
        this->m_model->day_length = DAY_LENGTH;
        this->m_model->start_time = (this->m_model->day_length / 3) * 1000;
        this->m_model->start_ticks = static_cast<int>(SDL_GetTicks());
        this->m_model->voxel_scene_w = INIT_WINDOW_WIDTH;
        this->m_model->voxel_scene_h = INIT_WINDOW_HEIGHT;
        this->m_model->scale = 1;
        this->m_model->is_ortho = false;
        this->m_model->fov = 65.f;
        SDL_snprintf(this->m_model->db_path, MAX_DB_PATH_LEN, "%s", "craft.db");
    }

}; // craft_impl

craft::craft(const std::string& title, const std::string& version, const int w, const int h)
    : m_pimpl{ std::make_unique<craft_impl>(cref(title), cref(version), w, h) } {
}

craft::~craft() = default;


/**
 * Run the craft-engine in a loop with SDL window open, compute the maze first
*/
bool craft::run(const std::function<int(int, int)>& get_int, std::mt19937& rng) const noexcept {

    if (!SDL_SetAppMetadata("Maze builder with voxels", mazes::VERSION.c_str(), ZACHS_GH_REPO)) {
        return SDL_APP_FAILURE;
    }

    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, ZACHS_GH_REPO);
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "flipsAndAle");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, "MIT License");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "simulation;game;voxel");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, mazes::VERSION.c_str());

    // SDL INITIALIZATION //
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_Init failed (%s)\n", SDL_GetError());
        return false;
    }

    this->m_pimpl->create_window_and_context();

    auto&& sdl_window = this->m_pimpl->m_model->window;

    if (!sdl_window) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_Window failed (%s)\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    SDL_ShowWindow(sdl_window);
    SDL_SetWindowRelativeMouseMode(sdl_window, false);
    SDL_SetWindowPosition(sdl_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

#if !defined(__EMSCRIPTEN__)
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "OpenGL loader failed (%s)\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
#endif

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

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

    // LOAD TEXTURES
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    load_png_texture("textures/texture.png");

    GLuint sign;
    glGenTextures(1, &sign);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, sign);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    load_png_texture("textures/sign.png");

    GLuint font;
    glGenTextures(1, &font);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, font);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    load_png_texture("textures/font.png");

    // Cubemaps
    static vector<string> cubemap_files = {"textures/right.jpg", "textures/left.jpg", 
        "textures/top.jpg", "textures/bottom.jpg", 
        "textures/front.jpg", "textures/back.jpg"};
    GLuint cubemap_texture_id = load_cubemap(cref(cubemap_files));

    // LOAD SHADERS 
    craft_impl::Attrib block_attrib = { 0 };
    craft_impl::Attrib line_attrib = { 0 };
    craft_impl::Attrib text_attrib = { 0 };
    craft_impl::Attrib screen_attrib = { 0 };
    craft_impl::Attrib blur_attrib = { 0 };
    craft_impl::Attrib skybox_attrib = { 0 };

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
    program = load_program("shaders/es/screen_vertex.es.glsl", "shaders/es/screen_fragment.es.glsl");
#else
    program = load_program("shaders/screen_vertex.glsl", "shaders/screen_fragment.glsl");
#endif
    screen_attrib.program = program;
    screen_attrib.position = 0;
    screen_attrib.uv = 1;
    screen_attrib.sampler = glGetUniformLocation(program, "screenTexture");
    screen_attrib.extra1 = glGetUniformLocation(program, "do_bloom");
    screen_attrib.extra2 = glGetUniformLocation(program, "exposure");
    screen_attrib.extra3 = glGetUniformLocation(program, "bloomBlur");

#if defined(__EMSCRIPTEN__)
    program = load_program("shaders/es/blur_vertex.es.glsl", "shaders/es/blur_fragment.es.glsl");
#else
    program = load_program("shaders/blur_vertex.glsl", "shaders/blur_fragment.glsl");
#endif
    blur_attrib.program = program;
    blur_attrib.position = 0;
    blur_attrib.uv = 1;
    blur_attrib.sampler = glGetUniformLocation(program, "image");
    blur_attrib.extra1 = glGetUniformLocation(program, "horizontal");
    blur_attrib.extra2 = glGetUniformLocation(program, "weight");
    const std::vector<GLfloat> WEIGHTS_IN_BLUR = {0.2270270270f, 0.1945945946f, 0.1216216216f, 0.0540540541f, 0.0162162162f};
    glUseProgram(program);
    glUniform1fv(blur_attrib.extra2, WEIGHTS_IN_BLUR.size(), WEIGHTS_IN_BLUR.data());
    glUseProgram(0);

#if defined(__EMSCRIPTEN__)
    program = load_program("shaders/es/skybox_vertex.es.glsl", "shaders/es/skybox_fragment.es.glsl");
#else
    program = load_program("shaders/skybox_vertex.glsl", "shaders/skybox_fragment.glsl");
#endif
    skybox_attrib.program = program;
    skybox_attrib.position = 0;
    skybox_attrib.matrix = glGetUniformLocation(program, "matrix");
    skybox_attrib.sampler = glGetUniformLocation(program, "skybox");

    // DEAR IMGUI INIT - Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
    ImGui::GetIO().IniFilename = nullptr;

    // Setup ImGui Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(sdl_window, this->m_pimpl->m_model->context);
    string glsl_version = "";
#if defined(__EMSCRIPTEN__)
    glsl_version = "#version 100";
#else
    glsl_version = "#version 130";
#endif
    ImGui_ImplOpenGL3_Init(glsl_version.c_str());
    ImGui::StyleColorsLight();
    ImFont* nunito_sans_font = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(NunitoSans_compressed_data, NunitoSans_compressed_size, 18.f);

    IM_ASSERT(nunito_sans_font != nullptr);

    // DATABASE INITIALIZATION 
    if (USE_CACHE) {
        db_enable();
        if (db_init(this->m_pimpl->m_model->db_path)) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "db_init failed\n");
            return false;
        }
    }

    // Init OpenGL fields
    // Vertex attributes for a quad that fills the entire screen in Normalized Device Coords
    static constexpr float quad_vertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    GLuint quad_vao = 0, quad_vbo = 0;
    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);
    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    static constexpr float skybox_vertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
    };

    // skybox VAO
    GLuint skybox_vao, skybox_vbo;
    glGenVertexArrays(1, &skybox_vao);
    glGenBuffers(1, &skybox_vbo);
    glBindVertexArray(skybox_vao);
    glBindBuffer(GL_ARRAY_BUFFER, skybox_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), &skybox_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glBindVertexArray(0);

    craft_impl::BloomTools bloom_tools{};

    craft_impl::Player* me = &m_pimpl->m_model->player;
    craft_impl::State* p_state = &m_pimpl->m_model->player.state;

    // LOAD STATE FROM DATABASE 
    int loaded = db_load_state(&p_state->x, &p_state->y, &p_state->z, &p_state->rx, &p_state->ry);
    if (!loaded) {
        p_state->y = this->m_pimpl->highest_block(p_state->x, p_state->z);
    }

    // Init some local vars for handling maze duties
    list<string> algo_list;
    for (auto i{ static_cast<int>(algo::BINARY_TREE) }; i < static_cast<int>(algo::TOTAL); ++i) {
        algo_list.push_back(to_string_from_algo(static_cast<algo>(i)));
    }

    // Make some references
    auto my_maze_type = to_algo_from_string(algo_list.front());
    auto&& gui = this->m_pimpl->m_gui;
    auto&& model = this->m_pimpl->m_model;

    mazes::lab my_mazes;

    // INITIALIZE WORKER THREADS
    m_pimpl->init_worker_threads(cref(my_mazes));

    me->id = 0;
    me->name = "Wade Watts";
    me->buffer = this->m_pimpl->gen_player_buffer(p_state->x, p_state->y, p_state->z, p_state->rx, p_state->ry);

    vector<uint8_t> current_maze_pixels;

    SDL_Log("CHECK_GL_ERR() prior to main loop\n");
    CHECK_GL_ERR();

    // LOCAL VARIABLES
    progress prog;
    uint64_t previous = SDL_GetTicks();
    double last_commit = SDL_GetTicks();
    int triangle_faces = 0;
    bool running = true;
    double time_step = 0;
    double time_accum = 0;

    // BEGIN EVENT LOOP
#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (running)
#endif
    {
        // FRAME RATE 
        uint64_t now = SDL_GetTicks();
        double elapsed = static_cast<double>(now - previous) / 1000.0;
        elapsed = SDL_min(elapsed, 0.2);
        elapsed = SDL_max(elapsed, 0.0);
        previous = now;

        // FLUSH DATABASE
        if (now - last_commit > COMMIT_INTERVAL) {
            db_commit();
            last_commit = now;
        }

        // Update player state
        p_state->t = static_cast<float>(elapsed);

        // Some GUI state variables
        static bool show_demo_window = false;

        // Handle SDL events and motion updates
        static bool window_resizes = true;
        static constexpr auto FIXED_TIME_STEP = 1.0 / 60.0;
        time_accum += elapsed;
        while (time_accum >= FIXED_TIME_STEP) {
            // Handle SDL events and motion (keyboard, mouse, etc.)
            running = this->m_pimpl->handle_events_and_motion(FIXED_TIME_STEP, ref(window_resizes));
            time_accum -= FIXED_TIME_STEP;
            time_step += FIXED_TIME_STEP;
        }

        if (model->create_radius != gui->view) {
            model->create_radius = gui->view;
            model->render_radius = gui->view;
            model->delete_radius = gui->view;
        }

        // Use ImGui for GUI size calculations
        int sdl_display_w, sdl_display_h;
        SDL_GetWindowSize(sdl_window, &sdl_display_w, &sdl_display_h);
        ImVec2 im_display_size = { static_cast<float>(sdl_display_w), static_cast<float>(sdl_display_h) };

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::PushFont(nunito_sans_font);

        // Show the big demo window?
        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        // Modal window with tabs
        if (!gui->capture_mouse) {
            ImGui::OpenPopup("Modal");
            SDL_SetWindowRelativeMouseMode(sdl_window, false);
        }

        if (ImGui::BeginPopupModal("Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            if (ImGui::BeginTabBar("Tabs")) {
                if (ImGui::BeginTabItem("Builder")) {
                    static unsigned int MAX_ROWS = 50;
                    if (ImGui::SliderInt("Rows", &gui->rows, 5, MAX_ROWS)) {

                    }
                    static unsigned int MAX_COLUMNS = 50;
                    if (ImGui::SliderInt("Columns", &gui->columns, 5, MAX_COLUMNS)) {

                    }
                    static unsigned int MAX_HEIGHT = 10;
                    if (ImGui::SliderInt("Height", &gui->height, 1, MAX_HEIGHT)) {

                    }

                    ImGui::TextColored(ImVec4(0.14f, 0.26f, 0.90f, 1.0f), "offset_x: %d", static_cast<int>(p_state->x));
                    ImGui::TextColored(ImVec4(0.14f, 0.26f, 0.90f, 1.0f), "offset_z: %d", static_cast<int>(p_state->z));

                    static unsigned int MAX_SEED_VAL = 100;
                    if (ImGui::SliderInt("Seed", &gui->seed, 0, MAX_SEED_VAL)) {
                        rng.seed(static_cast<unsigned long>(gui->seed));
                    }
                    ImGui::InputText("Tag", &gui->tag[0], MAX_SIGN_LENGTH);
                    ImGui::InputText("Outfile", &gui->outfile[0], IM_ARRAYSIZE(gui->outfile));
                    if (ImGui::TreeNode("Maze Algorithm")) {
                        auto preview{ gui->algo.c_str() };
                        ImGui::NewLine();
                        ImGuiComboFlags combo_flags = ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_WidthFitPreview;
                        if (ImGui::BeginCombo("algorithm", preview, combo_flags)) {
                            for (const auto& itr : algo_list) {
                                bool is_selected = (itr == gui->algo);
                                if (ImGui::Selectable(itr.c_str(), is_selected)) {
                                    gui->algo = itr;
                                    my_maze_type = to_algo_from_string(itr);
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
                            // Building maze here has the effect of computing its geometry on this thread
                            prog.reset();
                            prog.start();

                            auto next_maze_ptr = mazes::factory::create(
                                mazes::configurator().columns(gui->columns).rows(gui->rows).levels(gui->height)
                                .distances(false).seed(gui->seed)._algo(my_maze_type)
                                .block_id(items[model->item_index]));

                            if (!next_maze_ptr.has_value()) {
                                SDL_Log("Failed to create maze!");
                            }

                            my_mazes.set_levels(gui->height);

                            // Compute the geometry of the maze
                            vector<vector<uint32_t>> faces;
                            std::vector<std::tuple<int, int, int, int>> vertices;
                            auto s = mazes::stringz::stringify(next_maze_ptr.value());
                            mazes::stringz::objectify(cref(next_maze_ptr.value()), ref(vertices), ref(faces), s);
                            mazes::stringz::objectify(ref(my_mazes), s);
                            mazes::wavefront_object_helper woh{};
                            auto wavefront_obj_str = woh.to_wavefront_object_str(cref(vertices), cref(faces));

#if !defined(__EMSCRIPTEN__)
                            // Write immediately if not on the web
                            mazes::writer writer{};
                            writer.write_file(string(gui->outfile), cref(wavefront_obj_str));
#if defined(MAZE_DEBUG)
                            SDL_Log("Writing to file... %s\n", gui->outfile);
#endif
#endif
                            // The JSON data for the Web API - GET /mazes/
                            //this->m_pimpl->m_json_data = my_mazes.back()->to_json_str();
                            // Resetting the model reloads the chunks - show the new maze
                            this->m_pimpl->reset_model();
                            gui->reset();
                        }
                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.14f, 0.26f, 0.90f, 1.0f));
                        ImGui::Text(" => %s\n", gui->outfile);
                        ImGui::PopStyleColor();
                    } else {
                        // Disable the button
                        ImGui::BeginDisabled(true);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha | ImGuiTabItemFlags_None, ImGui::GetStyle().Alpha * 0.5f);

                        // Render the button - don't need to check if the button is pressed because it's disabled
                        ImGui::Button("Outfile?");

                        // Re-enable items and revert style change
                        ImGui::PopStyleVar();
                        ImGui::EndDisabled();
                    }

                    if (!my_mazes.empty()) {
                        // Show last maze compute time
                        ImGui::NewLine();
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.14f, 0.26f, 0.90f, 1.0f));
                        ImGui::Text("Elapsed %.5f ms", prog.elapsed());
                        ImGui::NewLine();
                        ImGui::PopStyleColor();
                    }

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Graphics")) {
                    ImGui::Checkbox("Dark Mode", &gui->color_mode_dark);
                    if (gui->color_mode_dark)
                        ImGui::StyleColorsDark();
                    else
                        ImGui::StyleColorsLight();

                    ImGui::SliderInt("View", &gui->view, 1, 24);

                    // Prevent setting SDL_Window settings every frame
                    static bool last_fullscreen = gui->fullscreen;
                    ImGui::Checkbox("Fullscreen (ESC to Exit)", &gui->fullscreen);
                    bool update_fullscreen = (last_fullscreen != gui->fullscreen) ? true : false;
                    last_fullscreen = gui->fullscreen;
                    if (update_fullscreen) {
                        SDL_SetWindowFullscreen(sdl_window, gui->fullscreen);
                        SDL_Log("Setting fullscreen to %d\n", gui->fullscreen);
                    }

                    static bool last_vsync = gui->vsync;
                    ImGui::Checkbox("VSYNC", &gui->vsync);
                    bool update_vsync = (last_vsync != gui->vsync) ? true : false;
                    last_vsync = gui->vsync;
                    if (update_vsync) {
                        SDL_GL_SetSwapInterval(gui->vsync);
                    }

                    ImGui::Checkbox("Show Items", &gui->show_items);
                    ImGui::Checkbox("Show Wireframes", &gui->show_wireframes);
                    ImGui::Checkbox("Show Crosshairs", &gui->show_crosshairs);
                    ImGui::Checkbox("Apply Bloom Effect", &gui->apply_bloom_effect);
                    ImGui::SliderFloat("Exp", &gui->exposure, 0.1f, 1.0f, "%.2f");

                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Commands")) {
                    ImGui::NewLine();
                    ImGui::Text("Commands:");
                    ImGui::Text("LMouse: Delete block");
                    ImGui::Text("RMouse: Build a block");
                    ImGui::Text("MMouse: Copy block type");
                    ImGui::Text("Spacebar: Jump");
                    ImGui::Text("Tab: Fly");
                    ImGui::Text("LShift: Zoom");
                    ImGui::Text("WASD: Movement");
                    ImGui::Text("Arrow Keys: Camera rotation");
                    ImGui::Text("F: Orthogonal projection");
                    ImGui::Text("E: Cycle Item");
                    ImGui::Text("R: Cycle Item");
                    ImGui::Text("T: Tag a block");
                    ImGui::Text("Control + Click: Place light");
                    ImGui::NewLine();
                    ImGui::EndTabItem();

                }
                ImGui::EndTabBar();
            } // Window tabs

            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
                gui->capture_mouse = true;
                SDL_SetWindowRelativeMouseMode(sdl_window, true);
            }
            ImGui::EndPopup();
        } // Modal window

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(im_display_size.x, im_display_size.y));
        ImGui::Begin("Voxels",
            nullptr,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoBringToFrontOnFocus
        );

        // PREPARE TO RENDER

        // Update Voxel window coords
        ImVec2 voxel_scene_size = ImGui::GetContentRegionAvail();
        int voxel_scene_w = static_cast<int>(voxel_scene_size.x);
        int voxel_scene_h = static_cast<int>(voxel_scene_size.y);
        model->voxel_scene_w = voxel_scene_w;
        model->voxel_scene_h = voxel_scene_h;

        // Check if scene size changed
        if (window_resizes) {
            window_resizes = false;
            // Delete existing FBO objects
            if (glIsTexture(bloom_tools.color_buffers[0]) && glIsTexture(bloom_tools.color_buffers[1])) {
                glDeleteTextures(2, bloom_tools.color_buffers);
                glDeleteTextures(1, &bloom_tools.color_final);
                glDeleteTextures(2, bloom_tools.color_buffers_pingpong);
                glDeleteFramebuffers(1, &bloom_tools.fbo_hdr);
                glDeleteRenderbuffers(1, &bloom_tools.rbo_bloom_depth);
                glDeleteFramebuffers(2, bloom_tools.color_buffers_pingpong);
                glDeleteFramebuffers(1, &bloom_tools.fbo_final);
            }
            bloom_tools.gen_framebuffers(sdl_display_w, sdl_display_h);
        }

        m_pimpl->delete_chunks();
        m_pimpl->del_buffer(me->buffer);
        me->buffer = this->m_pimpl->gen_player_buffer(p_state->x, p_state->y, p_state->z, p_state->rx, p_state->ry);

        // Bind the FBO that will store the 3D scene
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glViewport(0, 0, sdl_display_w, sdl_display_h);
        glBindFramebuffer(GL_FRAMEBUFFER, bloom_tools.fbo_hdr);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        triangle_faces = m_pimpl->render_chunks(&block_attrib, me, texture, cref(my_mazes));

        if (gui->show_items) {
            m_pimpl->render_item(&block_attrib, texture);
        }

        m_pimpl->render_signs(&text_attrib, me, sign);
        m_pimpl->render_sign(&text_attrib, me, sign);

        if (gui->show_wireframes) {
            m_pimpl->render_wireframe(&line_attrib, me);
        }

        if (gui->show_crosshairs) {
            m_pimpl->render_crosshairs(&line_attrib);
        }

        if (gui->show_info_text) {
            static constexpr auto TEXT_BUFFER_LENGTH = 1024u;
            char text_buffer[TEXT_BUFFER_LENGTH];

            float ts = 16.f * static_cast<float>(model->scale);
            float tx = ts / 1.45f;
            float ty = voxel_scene_h - ts * 1.5f;

            SDL_memset(text_buffer, 0, TEXT_BUFFER_LENGTH);
            SDL_snprintf(
                text_buffer, 1024,
                "%.3f ms/frame %.1f FPS",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            this->m_pimpl->render_text(&text_attrib, font, 0, tx, ty, ts, text_buffer);

            ty -= ts * 2.f;
            SDL_memset(text_buffer, 0, TEXT_BUFFER_LENGTH);
            SDL_snprintf(text_buffer, TEXT_BUFFER_LENGTH,
                "triangle faces %d", triangle_faces * 2);
            this->m_pimpl->render_text(&text_attrib, font, 0, tx, ty, ts, text_buffer);

            ty -= ts * 2.f;
            SDL_memset(text_buffer, 0, TEXT_BUFFER_LENGTH);
            SDL_snprintf(
                text_buffer, TEXT_BUFFER_LENGTH, "loc %d %d %d",
                this->m_pimpl->chunked(p_state->x),
                this->m_pimpl->chunked(p_state->y),
                this->m_pimpl->chunked(p_state->z));
            this->m_pimpl->render_text(&text_attrib, font, 0, tx, ty, ts, text_buffer);

            // Check the time
            int hour = static_cast<int>(this->m_pimpl->time_of_day()) * 24;
            char am_pm = hour < 12 ? 'a' : 'p';
            hour = hour % 12;
            hour = hour ? hour : 12;
            ty -= ts * 2.f;
            SDL_memset(text_buffer, 0, TEXT_BUFFER_LENGTH);
            SDL_snprintf(
                text_buffer, TEXT_BUFFER_LENGTH,
                "%d:%02d %cm",
                hour, static_cast<int>(this->m_pimpl->time_of_day() * 60), am_pm);
            this->m_pimpl->render_text(&text_attrib, font, 0, tx, ty, ts, text_buffer);

            ty -= ts * 2.f;
            SDL_memset(text_buffer, 0, TEXT_BUFFER_LENGTH);
            SDL_snprintf(text_buffer, TEXT_BUFFER_LENGTH,
                "chunks %d", model->chunk_count);
            this->m_pimpl->render_text(&text_attrib, font, 0, tx, ty, ts, text_buffer);
        }

        // Let the skybox pos.z coord determine depth test in shader
        glDepthFunc(GL_LEQUAL);

        // Skybox
        float sky_matrix[16];
        set_matrix_3d(
            sky_matrix, model->voxel_scene_w, model->voxel_scene_h,
            0.f, 0.f, 0.f, p_state->rx, p_state->ry, model->fov,
            0, model->render_radius);
        glUseProgram(skybox_attrib.program);
        glUniformMatrix4fv(skybox_attrib.matrix, 1, GL_FALSE, sky_matrix);
        glUniform1i(skybox_attrib.sampler, 0);
        glBindVertexArray(skybox_vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_texture_id);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Complete the pingpong buffer for the bloom effect
        glUseProgram(blur_attrib.program);
        for (auto i = 0; i < bloom_tools.NUM_FBO_ITERATIONS; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, bloom_tools.fbo_pingpong[bloom_tools.horizontal_blur]);
            glUniform1i(blur_attrib.extra1, bloom_tools.horizontal_blur);
            if (bloom_tools.first_iteration) {
                glUniform1i(blur_attrib.sampler, 0);
                glActiveTexture(GL_TEXTURE0);
                // Write to the floating-point buffer / COLOR_ATTACHMENT1 first iteration
                glBindTexture(GL_TEXTURE_2D, bloom_tools.color_buffers[1]);
                bloom_tools.first_iteration = false;
            } else {
                glBindTexture(GL_TEXTURE_2D, bloom_tools.color_buffers_pingpong[!bloom_tools.horizontal_blur]);
            }
            bloom_tools.horizontal_blur = !bloom_tools.horizontal_blur;
            glBindVertexArray(quad_vao);
            glDisable(GL_DEPTH_TEST);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        bloom_tools.first_iteration = true;

        // Post-processing the default frame buffer
        // Render HDR buffer to 2D quad and apply bloom filter
        glBindFramebuffer(GL_FRAMEBUFFER, bloom_tools.fbo_final);
        glUseProgram(screen_attrib.program);
        glUniform1i(screen_attrib.sampler, 0);
        glUniform1i(screen_attrib.extra3, 1);
        glBindVertexArray(quad_vao);
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bloom_tools.color_buffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bloom_tools.color_buffers_pingpong[!bloom_tools.horizontal_blur]);
        glUniform1i(screen_attrib.extra1, gui->apply_bloom_effect);
        // Exposure
        glUniform1f(screen_attrib.extra2, gui->exposure);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // Flip UV coordinates for the image
        ImVec2 uv0 = ImVec2(0.0f, 1.0f);
        ImVec2 uv1 = ImVec2(1.0f, 0.0f);
        ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(bloom_tools.color_final)), voxel_scene_size, uv0, uv1);
        ImGui::End();

        ImGui::PopFont();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(sdl_window);

        CHECK_GL_ERR();

    } // EVENT LOOP

#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_MAINLOOP_END;
    emscripten_cancel_main_loop();
#endif

    m_pimpl->cleanup_worker_threads();

    SDL_Log("Closing DB. . .\n");
    SDL_Log("Cleaning up ImGui objects. . .");
    SDL_Log("Cleaning up OpenGL objects. . .");
    SDL_Log("Cleaning up SDL objects. . .");

    db_save_state(p_state->x, p_state->y, p_state->z, p_state->rx, p_state->ry);
    db_close();
    db_disable();

    m_pimpl->delete_all_chunks();
    m_pimpl->delete_all_players();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    glDeleteTextures(1, &texture);
    glDeleteTextures(1, &font);
    glDeleteTextures(1, &sign);
    glDeleteTextures(1, &cubemap_texture_id);
    glDeleteRenderbuffers(1, &bloom_tools.rbo_bloom_depth);
    glDeleteFramebuffers(1, &bloom_tools.fbo_hdr);
    glDeleteFramebuffers(1, &bloom_tools.fbo_final);
    glDeleteFramebuffers(2, bloom_tools.fbo_pingpong);
    glDeleteTextures(2, bloom_tools.color_buffers);
    glDeleteTextures(2, bloom_tools.color_buffers_pingpong);
    glDeleteTextures(1, &bloom_tools.color_final);
    glDeleteVertexArrays(1, &quad_vao);
    glDeleteBuffers(1, &quad_vbo);
    glDeleteVertexArrays(1, &skybox_vao);
    glDeleteBuffers(1, &skybox_vbo);
    glDeleteProgram(block_attrib.program);
    glDeleteProgram(text_attrib.program);
    glDeleteProgram(line_attrib.program);
    glDeleteProgram(screen_attrib.program);
    glDeleteProgram(blur_attrib.program);
    glDeleteProgram(skybox_attrib.program);

    SDL_GL_DestroyContext(this->m_pimpl->m_model->context);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    return true;
}  // run

/**
 *
 *
 * @brief Used by Emscripten mostly to produce a JSON string containing the vertex data
 * @return returns JSON-encoded string: "{\"name\":\"MyMaze\", \"data\":\"v 1.0 1.0 0.0\\nv -1.0 1.0 0.0\\n...\"}";
 */
std::string craft::mazes() const noexcept {
    return this->m_pimpl->m_json_data;
}

/**
 * @brief Useful on mobile devices to flip mouse/finger capture
 *
 */
void craft::toggle_mouse() const noexcept {
    this->m_pimpl->m_gui->capture_mouse = !this->m_pimpl->m_gui->capture_mouse;
}
