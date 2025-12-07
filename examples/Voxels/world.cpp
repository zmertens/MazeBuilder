#include "world.h"

#include <noise/noise.h>

#include "command_queue.h"
#include "cube.h"
#include "db.h"
#include "resource_manager.h"
#include "font.h"
#include "item.h"
#include "matrix.h"
#include "player.h"
#include "texture.h"
#include "resource_identifiers.h"
#include "sdl_helper.h"
#include "shader.h"
#include "sign.h"

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>

std::string gl_error_checker(const char* file, const int line) noexcept
{
    GLenum error_code;
    std::string error_str;
    while ((error_code = glGetError()) != GL_NO_ERROR)
    {
        switch (error_code)
        {
        case GL_INVALID_ENUM: error_str += "INVALID_ENUM";
        case GL_INVALID_VALUE: error_str += "INVALID_VALUE";
        case GL_INVALID_OPERATION: error_str += "INVALID_OPERATION";
        case GL_OUT_OF_MEMORY: error_str += "OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION: error_str += "INVALID_FRAMEBUFFER_OPERATION";
        default: ;
        }
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                     "OpenGL ERROR: %s\n\t\tFILE: %s, LINE: %d\n", error_str.c_str(), file, line);
    }
    return error_code == GL_NO_ERROR ? "" : error_str;
}

world::world(SDL_Window* window, font_manager& fonts,
        player* p,
        shader_manager& shaders,
        texture_manager& textures)
    : m_window{window}
      , m_fonts{fonts}
      , m_shaders{shaders}
      , m_textures{textures}
      //   , mSceneGraph{}
      //   , mSceneLayers{}
      , m_command_queue{}
      , m_player{p}
      , m_block_attrib{}, m_line_attrib{}, m_text_attrib{}, m_sky_attrib{}
      , m_model{} // Initialize m_model to zero-initialize all members
{
}

world::~world()
{
    destroy_world();
}

void world::init() noexcept
{
    // Initialize model fields to ensure proper state
    m_model.chunk_count = 0;
    m_model.create_radius = CREATE_CHUNK_RADIUS;
    m_model.render_radius = RENDER_CHUNK_RADIUS;
    m_model.delete_radius = DELETE_CHUNK_RADIUS;
    m_model.sign_radius = RENDER_SIGN_RADIUS;
    m_model.flying = false;
    m_model.item_index = 0;
    m_model.is_ortho = false;
    m_model.fov = 65.0f;
    m_model.day_length = DAY_LENGTH;

    // Initialize window dimensions
    SDL_GetWindowSizeInPixels(m_window, &m_model.voxel_scene_w, &m_model.voxel_scene_h);

    // Set up OpenGL state (critical for rendering)
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glLogicOp(GL_INVERT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Initialize all chunks to have null/zero state
    for (auto& chunk : m_model.chunks)
    {
        chunk.map.data = nullptr;
        chunk.map.size = 0;
        chunk.lights.data = nullptr;
        chunk.lights.size = 0;
        chunk.signs.data = nullptr;
        chunk.signs.size = 0;
        chunk.signs.capacity = 0;
        chunk.buffer = 0;
        chunk.sign_buffer = 0;
    }

    init_worker_threads();

    // Force create initial chunks around player
    force_chunks(m_player);

    // Set player Y position to proper height above terrain
    m_player->s1.y = static_cast<float>(highest_block(m_player->s1.x, m_player->s1.z) + 2);

    SDL_Log("World initialized: Player starting position (%.2f, %.2f, %.2f), rotation (%.2f, %.2f)",
            m_player->s1.x, m_player->s1.y, m_player->s1.z, m_player->s1.rx, m_player->s1.ry);

    m_block_attrib.program = m_shaders.get(ShaderIdentifier::BLOCK_SHADER).get();
    m_block_attrib.position = 0;
    m_block_attrib.normal = 1;
    m_block_attrib.uv = 2;
    m_block_attrib.matrix = glGetUniformLocation(m_block_attrib.program, "matrix");
    m_block_attrib.sampler = glGetUniformLocation(m_block_attrib.program, "sampler");
    m_block_attrib.extra1 = glGetUniformLocation(m_block_attrib.program, "sky_sampler");
    m_block_attrib.extra2 = glGetUniformLocation(m_block_attrib.program, "daylight");
    m_block_attrib.extra3 = glGetUniformLocation(m_block_attrib.program, "fog_distance");
    m_block_attrib.extra4 = glGetUniformLocation(m_block_attrib.program, "is_ortho");
    m_block_attrib.camera = glGetUniformLocation(m_block_attrib.program, "camera");
    m_block_attrib.timer = glGetUniformLocation(m_block_attrib.program, "timer");

    m_line_attrib.program = m_shaders.get(ShaderIdentifier::LINE_SHADER).get();
    m_line_attrib.position = 0;
    m_line_attrib.matrix = glGetUniformLocation(m_line_attrib.program, "matrix");

    m_text_attrib.program = m_shaders.get(ShaderIdentifier::TEXT_SHADER).get();
    m_text_attrib.position = 0;
    m_text_attrib.uv = 1;
    m_text_attrib.matrix = glGetUniformLocation(m_text_attrib.program, "matrix");
    m_text_attrib.sampler = glGetUniformLocation(m_text_attrib.program, "sampler");
    m_text_attrib.extra1 = glGetUniformLocation(m_text_attrib.program, "is_sign");
}

void world::update(float delta_time, mazes::randomizer& rng) noexcept
{
    auto get_scale_factor = [this]() noexcept -> int
    {
        int window_width, window_height;
        int buffer_width, buffer_height;
        SDL_GetWindowSize(this->m_window, &window_width, &window_height);
        SDL_GetWindowSizeInPixels(this->m_window, &buffer_width, &buffer_height);
        return buffer_width / window_width;
    };

    m_model.scale = get_scale_factor();

    // Update window dimensions to ensure accurate viewport and projection matrix
    SDL_GetWindowSizeInPixels(m_window, &m_model.voxel_scene_w, &m_model.voxel_scene_h);

    delete_chunks();
    del_buffer(m_player->get_buffer());
    ensure_chunks(m_player);
}

void world::draw() const noexcept
{
    CHECK_GL_ERR();

    // Set viewport to match window dimensions
    int viewport_width, viewport_height;
    SDL_GetWindowSizeInPixels(m_window, &viewport_width, &viewport_height);
    glViewport(0, 0, viewport_width, viewport_height);

    // Verify OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glClearColor(0.53f, 0.81f, 0.92f, 1.0f); // Sky blue for better visibility
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get texture IDs from texture manager
    auto atlas_texture = m_textures.get(TextureIdentifier::ATLAS).get();
    auto signs_texture = m_textures.get(TextureIdentifier::SIGNS).get();

    // Debug: Verify textures are loaded
    static bool texture_logged = false;
    if (!texture_logged) {
        SDL_Log("Atlas texture ID: %u", atlas_texture);
        SDL_Log("Signs texture ID: %u", signs_texture);
        if (atlas_texture == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ERROR: Atlas texture not loaded!");
        }
        if (signs_texture == 0) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "ERROR: Signs texture not loaded!");
        }
        texture_logged = true;
    }

    auto triangle_faces = render_chunks(&m_block_attrib, m_player, atlas_texture);

    // Debug logging (can be commented out after testing)
    static int frame_count = 0;
    if (frame_count++ % 60 == 0) {
        SDL_Log("Frame %d: Rendered %d triangle faces, player at (%.2f, %.2f, %.2f), rot (%.2f, %.2f)",
                frame_count, triangle_faces,
                m_player->s1.x, m_player->s1.y, m_player->s1.z,
                m_player->s1.rx, m_player->s1.ry);
    }

    render_item(&m_block_attrib, atlas_texture);

    render_signs(&m_text_attrib, m_player, signs_texture);
    render_sign(&m_text_attrib, m_player, signs_texture);

    render_wireframe(&m_line_attrib, m_player);

    render_crosshairs(&m_line_attrib);
}

command_queue& world::get_command_queue() noexcept
{
    return m_command_queue;
}

void world::destroy_world()
{
    // Cleanup worker threads and chunks
    cleanup_worker_threads();
    delete_all_chunks();
    delete_all_players();
}

void world::handle_event(SDL_Event& event) noexcept
{
    if (event.type == SDL_EVENT_QUIT)
    {
    }
}

void set_player(player* player)
{
}

void world::create_world(int p, int q, world_func func, Map* m, int chunk_size) noexcept
{
    int pad = 1;
    for (int dx = -pad; dx < chunk_size + pad; dx++)
    {
        for (int dz = -pad; dz < chunk_size + pad; dz++)
        {
            int flag = 1;
            if (dx < 0 || dz < 0 || dx >= chunk_size || dz >= chunk_size)
            {
                flag = -1;
            }
            int x = p * chunk_size + dx;
            int z = q * chunk_size + dz;

            // Build the environment
            float f = simplex2(static_cast<float>(x) * 0.01, static_cast<float>(z) * 0.01, 4, 0.5, 2);
            float g = simplex2(static_cast<float>(-x) * 0.01, static_cast<float>(-z) * 0.01, 2, 0.9, 2);
            int mh = g * 32 + 16;
            int h = f * mh;
            int w = 1;
            int t = 12;
            if (h <= t)
            {
                h = t;
                w = 2;
            }

            static constexpr auto PLANT_HEIGHT_MAX = 2;

            // Maze
            //const auto& block = mazes.find(x, z);
            //if (block.has_value()) {
            //    const auto& [rows, cols, levels, t] = block.value();
            //    for (auto y = 0; y < levels + PLANT_HEIGHT_MAX + 1; y++) {
            //        func(rows, y, cols, t * flag, m);
            //    }
            //    continue;
            //}

            // sand and grass terrain
            for (int y = 0; y < PLANT_HEIGHT_MAX; y++)
            {
                func(x, y, z, w * flag, m);
            }

            if (w == 1)
            {
                // grass
                if (simplex2(-x * 0.1, z * 0.1, 4, 0.8, 2) > 0.6)
                {
                    func(x, PLANT_HEIGHT_MAX, z, 17 * flag, m);
                }
                // flowers
                if (simplex2(x * 0.05, -z * 0.05, 4, 0.8, 2) > 0.7)
                {
                    int w = 18 + simplex2(x * 0.1, z * 0.1, 4, 0.8, 2) * 7;
                    func(x, PLANT_HEIGHT_MAX, z, w * flag, m);
                }
                // trees
                // Check that the tree fits in the chunk
                bool ok = true;
                if (dx - 3 < 0 || dz - 3 < 0 || dx + 4 > chunk_size || dz + 4 > chunk_size)
                {
                    ok = false;
                }
                if (ok && simplex2(x, z, 6, 0.5, 2) > 0.84)
                {
                    // Generate canopy for tree (leaves)
                    for (int y = PLANT_HEIGHT_MAX + 3; y < PLANT_HEIGHT_MAX + 8; y++)
                    {
                        for (int ox = -3; ox <= 3; ox++)
                        {
                            for (int oz = -3; oz <= 3; oz++)
                            {
                                int d = (ox * ox) + (oz * oz) + (y - (PLANT_HEIGHT_MAX + 4)) * (y - (PLANT_HEIGHT_MAX +
                                    4));
                                if (d < 11)
                                {
                                    func(x + ox, y, z + oz, 15, m);
                                }
                            }
                        }
                    }
                    // Generate the tree trunk
                    for (int y = PLANT_HEIGHT_MAX; y < PLANT_HEIGHT_MAX + 7; y++)
                    {
                        func(x, y, z, 5, m);
                    }
                }
            }
            // clouds
            for (int y = 64; y < 72; y++)
            {
                if (simplex3(x * 0.01, y * 0.1, z * 0.01, 8, 0.5, 2) > 0.75)
                {
                    func(x, y, z, 16 * flag, m);
                }
            }
        }
    }
} // create_world

bool world::worker_run(void* arg) noexcept
{
    auto* worker = reinterpret_cast<Worker*>(arg);
    while (true)
    {
        while (worker->state != WORKER_BUSY && !worker->should_stop)
        {
            std::unique_lock<std::mutex> my_lock(worker->mtx);
            worker->cnd.wait(my_lock);
        }
        if (worker->should_stop)
        {
            break;
        }
        WorkerItem* worker_item = &worker->item;
        if (worker_item->load)
        {
            this->load_chunk(worker_item);
        }

        this->compute_chunk(worker_item);

        worker->mtx.lock();
        worker->state = WORKER_DONE;
        worker->mtx.unlock();
    }
    return true;
} // worker_run

void world::init_worker_threads() noexcept
{
    this->m_model.workers.reserve(NUM_WORKERS);
    for (int i = 0; i < NUM_WORKERS; i++)
    {
        auto worker = std::make_unique<Worker>();
        worker->index = i;
        worker->state = WORKER_IDLE;
        this->m_model.workers.emplace_back(std::move(worker));
        Worker* worker_ptr = this->m_model.workers.back().get();
        worker_ptr->thrd = std::thread([this, worker_ptr]() { this->worker_run(worker_ptr); });
    }
}

void world::cleanup_worker_threads() noexcept
{
    // signal all worker threads to stop
    for (auto&& w : m_model.workers)
    {
        w->mtx.lock();
        w->should_stop = true;
        w->cnd.notify_one();
        w->mtx.unlock();
    }
    // Wait for threads to join
    for (auto&& w : this->m_model.workers)
    {
        // Wait for the thread to complete its execution
        w->thrd.join();

        SDL_Log("Worker thread %d finished!", w->index);
    }
    // Clear the vector after all threads have been joined
    this->m_model.workers.clear();
}

void world::del_buffer(const std::uint32_t buffer) const noexcept
{
    glDeleteBuffers(1, &buffer);
}

std::uint32_t world::gen_buffer(const std::size_t size, const float* data) const noexcept
{
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizei>(size), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return buffer;
}

float* world::malloc_faces(const std::size_t components, const std::size_t faces) const noexcept
{
    return static_cast<GLfloat*>(SDL_malloc(sizeof(GLfloat) * 6 * components * faces));
}

/**
 * Generate a buffer for faces - data is not freed here
 */
std::uint32_t world::gen_faces(const std::size_t components, const std::size_t faces, const float* data) const noexcept
{
    const GLuint buffer = this->gen_buffer(sizeof(GLfloat) * 6 * components * faces, data);
    return buffer;
}

int world::chunked(const float x) const noexcept
{
    return static_cast<int>(SDL_floorf(SDL_roundf(x) / static_cast<float>(BUILD_CHUNK_SIZE)));
}

double world::get_time() const noexcept
{
    return (static_cast<double>(SDL_GetTicks()) + static_cast<double>(m_model.start_time) - static_cast<double>(m_model.
        start_ticks)) / 1000.0;
}

float world::time_of_day() const noexcept
{
    if (m_model.day_length <= 0)
    {
        return 0.5f;
    }
    auto t = static_cast<float>(get_time());
    t /= static_cast<float>(m_model.day_length);
    t -= static_cast<float>(static_cast<int>(t));
    return t;
}

float world::get_daylight() const noexcept
{
    if (const float timer = time_of_day(); timer < 0.5)
    {
        const float t = (timer - 0.25f) * 100.f;
        return 1 / (1 + SDL_powf(2.f, -t));
    }
    else
    {
        const float t = (timer - 0.85f) * 100.f;
        return 1.f - 1.f / (1.f + SDL_powf(2.f, -t));
    }
}

void world::compute_sight_vector(const float rx, const float ry, float* vx, float* vy, float* vz) const noexcept
{
    float m = SDL_cosf(ry);
    *vx = SDL_cosf(rx - static_cast<float>(RADIANS(90))) * m;
    *vy = SDL_sinf(ry);
    *vz = SDL_sinf(rx - static_cast<float>(RADIANS(90))) * m;
}

void world::compute_motion_vector(const int flying, const int sz, const int sx, const float rx, const float ry,
                                  float* vx, float* vy, float* vz) const noexcept
{
    *vx = 0;
    *vy = 0;
    *vz = 0;
    if (!sz && !sx)
    {
        return;
    }
    const float strafe = SDL_atan2f(static_cast<float>(sz), static_cast<float>(sx));
    if (flying)
    {
        float m = SDL_cosf(ry);
        float y = SDL_sinf(ry);
        if (sx)
        {
            if (!sz)
            {
                y = 0;
            }
            m = 1;
        }
        if (sz > 0)
        {
            y = -y;
        }
        *vx = SDL_cosf(rx + strafe) * m;
        *vy = y;
        *vz = SDL_sinf(rx + strafe) * m;
    }
    else
    {
        *vx = SDL_cosf(rx + strafe);
        *vy = 0;
        *vz = SDL_sinf(rx + strafe);
    }
}

std::uint32_t world::gen_crosshair_buffer() const noexcept
{
    const float x = static_cast<float>(m_model.voxel_scene_w) / 2.0f;
    const float y = static_cast<float>(m_model.voxel_scene_h) / 2.0f;
    const float p = 10.f * static_cast<float>(m_model.scale);
    const float data[] = {
        x, y - p, x, y + p,
        x - p, y, x + p, y
    };
    return gen_buffer(sizeof(data), data);
}

std::uint32_t world::gen_wireframe_buffer(float x, float y, float z, float n) const noexcept
{
    float data[72];
    // cube.h -> make_cube_wireframe
    make_cube_wireframe(data, x, y, z, n);
    return gen_buffer(sizeof(data), data);
}

std::uint32_t world::gen_cube_buffer(const float x, float y, float z, float n, int w) const noexcept
{
    GLfloat* data = malloc_faces(10, 6);
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

std::uint32_t world::gen_plant_buffer(const float x, const float y, const float z, const float n,
                                      const int w) const noexcept
{
    GLfloat* data = malloc_faces(10, 4);
    float ao = 0;
    float light = 1;
    make_plant(data, ao, light, x, y, z, n, w, 45);
    return gen_faces(10, 4, data);
}

std::uint32_t world::gen_player_buffer(const float x, const float y, const float z, const float rx,
                                       const float ry) const noexcept
{
    GLfloat* data = malloc_faces(10, 6);
    make_player(data, x, y, z, rx, ry);
    return gen_faces(10, 6, data);
}

std::uint32_t world::gen_text_buffer(float x, const float y, const float n, const std::string_view text) const noexcept
{
    const auto length = static_cast<GLsizei>(text.size());
    GLfloat* data = malloc_faces(4, length);
    for (int i = 0; i < length; i++)
    {
        make_character(data + i * 24, x, y, n / 2, n, text[i]);
        x += n;
    }
    return gen_faces(4, length, data);
}

void world::draw_triangles_3d_ao(const Attrib* attrib, const std::uint32_t buffer, const int count) const noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glEnableVertexAttribArray(attrib->normal);
    glEnableVertexAttribArray(attrib->uv);
    glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 10, nullptr);
    glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 10, reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 3));
    glVertexAttribPointer(attrib->uv, 4, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 10, reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 6));
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(attrib->position);
    glDisableVertexAttribArray(attrib->normal);
    glDisableVertexAttribArray(attrib->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void world::draw_triangles_3d_text(const Attrib* attrib, const std::uint32_t buffer, const int count) const noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glEnableVertexAttribArray(attrib->uv);
    glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 5, nullptr);
    glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 5, reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 3));
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableVertexAttribArray(attrib->position);
    glDisableVertexAttribArray(attrib->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void world::draw_triangles_3d(const Attrib* attrib, const std::uint32_t buffer, const int count) const noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    glEnableVertexAttribArray(attrib->position);

    glEnableVertexAttribArray(attrib->normal);
    glEnableVertexAttribArray(attrib->uv);

    glVertexAttribPointer(attrib->position, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, nullptr);
    glVertexAttribPointer(attrib->normal, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8,
                          reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 3));
    glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8,
                          reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 6));

    glDrawArrays(GL_TRIANGLES, 0, count);

    glDisableVertexAttribArray(attrib->position);
    glDisableVertexAttribArray(attrib->normal);
    glDisableVertexAttribArray(attrib->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void world::draw_triangles_2d(const Attrib* attrib, const std::uint32_t buffer, const std::size_t count) const noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glEnableVertexAttribArray(attrib->uv);
    glVertexAttribPointer(attrib->position, 2, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 4, 0);
    glVertexAttribPointer(attrib->uv, 2, GL_FLOAT, GL_FALSE,
                          sizeof(GLfloat) * 4, reinterpret_cast<GLvoid*>(sizeof(GLfloat) * 2));
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(count));
    glDisableVertexAttribArray(attrib->position);
    glDisableVertexAttribArray(attrib->uv);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void world::draw_lines(const Attrib* attrib, const std::uint32_t buffer, const int components,
                       const int count) const noexcept
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(attrib->position);
    glVertexAttribPointer(
        attrib->position, components, GL_FLOAT, GL_FALSE, 0, 0);
    glDrawArrays(GL_LINES, 0, count);
    glDisableVertexAttribArray(attrib->position);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void world::draw_chunk(const Attrib* attrib, const Chunk* chunk) const noexcept
{
    draw_triangles_3d_ao(attrib, chunk->buffer, chunk->faces * 6);
}

void world::draw_item(const Attrib* attrib, const GLuint buffer, const int count) const noexcept
{
    draw_triangles_3d_ao(attrib, buffer, count);
}

void world::draw_text(const Attrib* attrib, const std::uint32_t buffer, const std::size_t length) const noexcept
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_triangles_2d(attrib, buffer, length * 6);
    glDisable(GL_BLEND);
}

void world::draw_signs(const Attrib* attrib, const Chunk* chunk) const noexcept
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-8, -1024);
    draw_triangles_3d_text(attrib, chunk->sign_buffer, chunk->sign_faces * 6);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void world::draw_sign(const Attrib* attrib, const std::uint32_t buffer, const int length) const noexcept
{
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-8, -1024);
    draw_triangles_3d_text(attrib, buffer, length * 6);
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void world::draw_cube(const Attrib* attrib, const std::uint32_t buffer) const noexcept
{
    draw_item(attrib, buffer, 36);
}

void world::draw_plant(const Attrib* attrib, const std::uint32_t buffer) const noexcept
{
    draw_item(attrib, buffer, 24);
}

void world::draw_player(const Attrib* attrib, const player* _player) const noexcept
{
    draw_cube(attrib, _player->get_buffer());
}

const player* world::find_player(const int id) const noexcept
{
    for (int i = 0; i < this->m_model.player_count; i++)
    {
        if (auto* p = &m_player; m_player->is_active())
        {
            return *p;
        }
    }
    return nullptr;
}

void world::delete_all_players() noexcept
{
    for (int i = 0; i < m_model.player_count; i++)
    {
        this->del_buffer(m_player->get_buffer());
    }
    m_model.player_count = 0;
}

std::optional<world::Chunk*> world::find_chunk(const int p, const int q) const noexcept
{
    for (int i = 0; i < m_model.chunk_count; i++)
    {
        Chunk* chunk = const_cast<Chunk*>(m_model.chunks + i);
        if (chunk->p == p && chunk->q == q)
        {
            return chunk;
        }
    }
    return std::nullopt;
}

int world::chunk_distance(const Chunk* chunk, const int p, const int q) noexcept
{
    const int dp = SDL_abs(chunk->p - p);
    const int dq = SDL_abs(chunk->q - q);
    return SDL_max(dp, dq);
}

int world::chunk_visible(float planes[6][4], int p, int q, const int miny, const int maxy) const noexcept
{
    auto miny_f = static_cast<float>(miny);
    auto maxy_f = static_cast<float>(maxy);
    auto x = static_cast<float>(p * BUILD_CHUNK_SIZE - 1);
    float z = static_cast<float>(q * BUILD_CHUNK_SIZE - 1);
    float d = static_cast<float>(BUILD_CHUNK_SIZE + 1);
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
    int n = this->m_model.is_ortho ? 4 : 6;
    for (int i = 0; i < n; i++)
    {
        int in = 0;
        int out = 0;
        for (int j = 0; j < 8; j++)
        {
            float d =
                planes[i][0] * points[j][0] +
                planes[i][1] * points[j][1] +
                planes[i][2] * points[j][2] +
                planes[i][3];
            if (d < 0)
            {
                out++;
            }
            else
            {
                in++;
            }
            if (in && out)
            {
                break;
            }
        }
        if (in == 0)
        {
            return 0;
        }
    }
    return 1;
} // chunk_visible

int world::highest_block(const float x, const float z) const noexcept
{
    int result = -1;
    int nx = static_cast<int>(SDL_roundf(x));
    int nz = static_cast<int>(SDL_roundf(z));
    int p = chunked(x);
    int q = chunked(z);
    auto chunk = find_chunk(p, q);
    if (chunk.has_value())
    {
        const Map* map = &chunk.value()->map;
        MAP_FOR_EACH(map, ex, ey, ez, ew)
            {
                // item.h -> is_obstacle
                if (is_obstacle(ew) && ex == nx && ez == nz)
                {
                    result = SDL_max(result, ey);
                }
            }
        END_MAP_FOR_EACH;
    }
    return result;
}

int world::_hit_test(const Map* map, const float max_distance, const int previous,
                     float x, float y, float z, float vx, float vy, float vz, int* hx, int* hy, int* hz) noexcept
{
    static constexpr int m = 32;
    int px = 0;
    int py = 0;
    int pz = 0;
    for (int i = 0; i < max_distance * m; i++)
    {
        int nx = SDL_lroundf(x);
        int ny = SDL_lroundf(y);
        int nz = SDL_lroundf(z);
        if (nx != px || ny != py || nz != pz)
        {
            int hw = map_get(map, nx, ny, nz);
            if (hw > 0)
            {
                if (previous)
                {
                    *hx = px;
                    *hy = py;
                    *hz = pz;
                }
                else
                {
                    *hx = nx;
                    *hy = ny;
                    *hz = nz;
                }
                return hw;
            }
            px = nx;
            py = ny;
            pz = nz;
        }
        x += vx / m;
        y += vy / m;
        z += vz / m;
    }
    return 0;
} // _hit_test

int world::hit_test(const int previous, const float x, const float y,
                    float z, float rx, float ry, int* bx, int* by, int* bz) const noexcept
{
    int result = 0;
    float best = 0;
    const int p = chunked(x);
    const int q = chunked(z);
    float vx, vy, vz;

    compute_sight_vector(rx, ry, &vx, &vy, &vz);

    for (int i = 0; i < m_model.chunk_count; i++)
    {
        const Chunk* chunk = m_model.chunks + i;
        if (chunk_distance(chunk, p, q) > 1)
        {
            continue;
        }
        int hx, hy, hz;
        const int hw = _hit_test(&chunk->map, 8, previous,
                                 x, y, z, vx, vy, vz, &hx, &hy, &hz);
        if (hw > 0)
        {
            if (float d = SDL_sqrtf(SDL_powf(hx - x, 2) + SDL_powf(hy - y, 2) + SDL_powf(hz - z, 2)); best == 0 || d <
                best)
            {
                best = d;
                *bx = hx;
                *by = hy;
                *bz = hz;
                result = hw;
            }
        }
    }
    return result;
} // hit_test

int world::hit_test_face(player* _player, int* x, int* y, int* z, int* face) const noexcept
{
    const player::state* s = &_player->s1;
    // item.h -> is_obstacle
    if (int w = this->hit_test(0, s->x, s->y, s->z, s->rx, s->ry, x, y, z); is_obstacle(w))
    {
        int hx, hy, hz;

        hit_test(1, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);

        int dx = hx - *x;
        int dy = hy - *y;
        int dz = hz - *z;
        if (dx == -1 && dy == 0 && dz == 0)
        {
            *face = 0;
            return 1;
        }
        if (dx == 1 && dy == 0 && dz == 0)
        {
            *face = 1;
            return 1;
        }
        if (dx == 0 && dy == 0 && dz == -1)
        {
            *face = 2;
            return 1;
        }
        if (dx == 0 && dy == 0 && dz == 1)
        {
            *face = 3;
            return 1;
        }
        if (dx == 0 && dy == 1 && dz == 0)
        {
            float degrees = SDL_roundf(static_cast<float>(DEGREES(SDL_atan2(s->x - hx, s->z - hz))));
            if (degrees < 0.f)
            {
                degrees += 360.f;
            }
            int top = static_cast<int>(((degrees + 45.f) / 90.f)) % 4;
            *face = 4 + top;
            return 1;
        }
    }
    return 0;
}

int world::collide(int height, float* x, float* y, float* z) const noexcept
{
    int result = 0;
    int p = this->chunked(*x);
    int q = this->chunked(*z);
    auto chunk_opt = find_chunk(p, q);
    if (!chunk_opt.has_value())
    {
        SDL_Log("Could find chunk: %d %d", p, q);
        return result;
    }
    const Chunk* chunk = chunk_opt.value();
    const Map* map = &chunk->map;
    int nx = static_cast<int>(SDL_roundf(*x));
    int ny = static_cast<int>(SDL_roundf(*y));
    int nz = static_cast<int>(SDL_roundf(*z));
    float px = *x - nx;
    float py = *y - ny;
    float pz = *z - nz;
    float pad = 0.25;
    for (int dy = 0; dy < height; dy++)
    {
        // item.h -> is_obstacle
        if (px < -pad && is_obstacle(map_get(map, nx - 1, ny - dy, nz)))
        {
            *x = nx - pad;
        }
        if (px > pad && is_obstacle(map_get(map, nx + 1, ny - dy, nz)))
        {
            *x = nx + pad;
        }
        if (py < -pad && is_obstacle(map_get(map, nx, ny - dy - 1, nz)))
        {
            *y = ny - pad;
            result = 1;
        }
        if (py > pad && is_obstacle(map_get(map, nx, ny - dy + 1, nz)))
        {
            *y = ny + pad;
            result = 1;
        }
        if (pz < -pad && is_obstacle(map_get(map, nx, ny - dy, nz - 1)))
        {
            *z = nz - pad;
        }
        if (pz > pad && is_obstacle(map_get(map, nx, ny - dy, nz + 1)))
        {
            *z = nz + pad;
        }
    }
    return result;
}

int world::player_intersects_block(int height, float x, float y, float z, int hx, int hy, int hz) const noexcept
{
    int nx = static_cast<int>(SDL_roundf(x));
    int ny = static_cast<int>(SDL_roundf(y));
    int nz = static_cast<int>(SDL_roundf(z));
    for (int i = 0; i < height; i++)
    {
        if (nx == hx && ny - i == hy && nz == hz)
        {
            return 1;
        }
    }
    return 0;
}

int world::_gen_sign_buffer(float* data, float x, float y, float z, int face, std::string_view text) const noexcept
{
    auto tokenize = [](char* str, const char* delim, char** key)-> char*
    {
        char* result;
        if (str == nullptr)
        {
            str = *key;
        }
        str += strspn(str, delim);
        if (*str == '\0')
        {
            return nullptr;
        }
        result = str;
        str += strcspn(str, delim);
        if (*str)
        {
            *str++ = '\0';
        }
        *key = str;
        return result;
    };

    auto char_width = [](const char input)
    {
        static const int lookup[128] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            4, 2, 4, 7, 6, 9, 7, 2, 3, 3, 4, 6, 3, 5, 2, 7,
            6, 3, 6, 6, 6, 6, 6, 6, 6, 6, 2, 3, 5, 6, 5, 7,
            8, 6, 6, 6, 6, 6, 6, 6, 6, 4, 6, 6, 5, 8, 8, 6,
            6, 7, 6, 6, 6, 6, 8, 10, 8, 6, 6, 3, 6, 3, 6, 6,
            4, 7, 6, 6, 6, 6, 5, 6, 6, 2, 5, 5, 2, 9, 6, 6,
            6, 6, 6, 6, 5, 6, 6, 6, 6, 6, 6, 4, 2, 5, 7, 0
        };
        return lookup[input];
    };

    auto string_width = [=](const char* input)
    {
        int result = 0;
        const std::size_t length = SDL_strlen(input);
        for (int i = 0; i < length; i++)
        {
            result += char_width(input[i]);
        }
        return result;
    };

    auto wrap = [=](const char* input, int max_width, char* output, int max_length)
    {
        *output = '\0';
        auto* str = static_cast<char*>(SDL_malloc(sizeof(char) * (strlen(input) + 1)));
        SDL_strlcpy(str, input, SDL_strlen(input) + 1);
        int space_width = char_width(' ');
        int line_number = 0;
        char *key1, *key2;
        char* line = tokenize(str, "\r\n", &key1);
        while (line)
        {
            int line_width = 0;
            const char* token = tokenize(line, " ", &key2);
            while (token)
            {
                int token_width = string_width(token);
                if (line_width)
                {
                    if (line_width + token_width > max_width)
                    {
                        line_width = 0;
                        line_number++;
                        strncat(output, "\n", max_length - strlen(output) - 1);
                    }
                    else
                    {
                        strncat(output, " ", max_length - strlen(output) - 1);
                    }
                }
                strncat(output, token, max_length - strlen(output) - 1);
                line_width += token_width + space_width;
                token = tokenize(NULL, " ", &key2);
            }
            line_number++;
            strncat(output, "\n", max_length - strlen(output) - 1);
            line = tokenize(NULL, "\r\n", &key1);
        }
        SDL_free(str);
        return line_number;
    };

    static constexpr int glyph_dx[8] = {0, 0, -1, 1, 1, 0, -1, 0};
    static constexpr int glyph_dz[8] = {1, -1, 0, 0, 0, -1, 0, 1};
    static constexpr int line_dx[8] = {0, 0, 0, 0, 0, 1, 0, -1};
    static constexpr int line_dy[8] = {-1, -1, -1, -1, 0, 0, 0, 0};
    static constexpr int line_dz[8] = {0, 0, 0, 0, 1, 0, -1, 0};
    if (face < 0 || face >= 8)
    {
        return 0;
    }
    int count = 0;
    float max_width = 64.f;
    float line_height = 1.25f;
    char lines[1024];
    int rows = wrap(text.data(), static_cast<int>(max_width), lines, 1024);
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
    while (line)
    {
        size_t length = SDL_strlen(line);
        int line_width = string_width(line);
        line_width = static_cast<int>(SDL_min(line_width, max_width));
        float rx = sx - dx * line_width / max_width / 2;
        float ry = sy;
        float rz = sz - dz * line_width / max_width / 2;
        for (int i = 0; i < length; i++)
        {
            int width = char_width(line[i]);
            line_width -= width;
            if (line_width < 0)
            {
                break;
            }
            rx += dx * width / max_width / 2;
            rz += dz * width / max_width / 2;
            if (line[i] != ' ')
            {
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
        line = tokenize(nullptr, "\n", &key);
        rows--;
        if (rows <= 0)
        {
            break;
        }
    }
    return count;
}

void world::gen_sign_buffer(Chunk* chunk) const noexcept
{
    const SignList* signs = &chunk->signs;

    // first pass - count characters
    std::size_t max_faces = 0;
    for (int i = 0; i < signs->size; i++)
    {
        Sign* e = signs->data + i;
        max_faces += SDL_strlen(e->text);
    }

    // second pass - generate geometry
    GLfloat* data = malloc_faces(5, max_faces);
    std::size_t faces = 0;
    for (int i = 0; i < signs->size; i++)
    {
        Sign* e = signs->data + i;
        faces += static_cast<std::size_t>(_gen_sign_buffer(data + static_cast<int>(faces) * 30,
                                                           static_cast<float>(e->x),
                                                           static_cast<float>(e->y),
                                                           static_cast<float>(e->z), e->face, e->text));
    }

    this->del_buffer(chunk->sign_buffer);
    chunk->sign_buffer = gen_faces(5, static_cast<GLsizei>(faces), data);
    chunk->sign_faces = static_cast<int>(faces);
}

int world::has_lights(Chunk* chunk) const noexcept
{
    for (int dp = -1; dp <= 1; dp++)
    {
        for (int dq = -1; dq <= 1; dq++)
        {
            Chunk* other = chunk;
            if (dp || dq)
            {
                auto other_opt = this->find_chunk(chunk->p + dp, chunk->q + dq);
                if (!other_opt.has_value())
                {
                    other = nullptr;
                }
                else
                {
                    other = other_opt.value();
                }
            }
            if (!other)
            {
                continue;
            }
            if (Map* map = &other->lights; map->size)
            {
                return 1;
            }
        }
    }
    return 0;
}

void world::dirty_chunk(Chunk* chunk) const noexcept
{
    chunk->dirty = 1;
    if (has_lights(chunk))
    {
        for (int dp = -1; dp <= 1; dp++)
        {
            for (int dq = -1; dq <= 1; dq++)
            {
                auto other_opt = find_chunk(chunk->p + dp, chunk->q + dq);
                if (other_opt.has_value())
                {
                    other_opt.value()->dirty = 1;
                }
            }
        }
    }
}

void world::occlusion(char neighbors[27], char lights[27], float shades[27], float ao[6][4],
                      float light[6][4]) const noexcept
{
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
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            int corner = neighbors[lookup3[i][j][0]];
            int side1 = neighbors[lookup3[i][j][1]];
            int side2 = neighbors[lookup3[i][j][2]];
            int value = side1 && side2 ? 3 : corner + side1 + side2;
            float shade_sum = 0;
            float light_sum = 0;
            int is_light = lights[13] == 15;
            for (int k = 0; k < 4; k++)
            {
                shade_sum += shades[lookup4[i][j][k]];
                light_sum += lights[lookup4[i][j][k]];
            }
            if (is_light)
            {
                light_sum = 15 * 4 * 10;
            }
            float total = curve[value] + shade_sum / 4.0f;
            ao[i][j] = SDL_min(total, 1.0f);
            light[i][j] = light_sum / 15.0f / 4.0f;
        }
    }
} // occlusion

void world::light_fill(char* opaque, char* light, int x, int y, int z, int w, int force) const noexcept
{
#define XZ_SIZE (BUILD_CHUNK_SIZE * 3 + 2)
#define XZ_LO (BUILD_CHUNK_SIZE)
#define XZ_HI (BUILD_CHUNK_SIZE * 2 + 1)
#define Y_SIZE 258
#define XYZ(x, y, z) ((y) * XZ_SIZE * XZ_SIZE + (x) * XZ_SIZE + (z))
#define XZ(x, z) ((x) * XZ_SIZE + (z))
    if (x + w < XZ_LO || z + w < XZ_LO)
    {
        return;
    }
    if (x - w > XZ_HI || z - w > XZ_HI)
    {
        return;
    }
    if (y < 0 || y >= Y_SIZE)
    {
        return;
    }
    if (light[XYZ(x, y, z)] >= w)
    {
        return;
    }
    if (!force && opaque[XYZ(x, y, z)])
    {
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
void world::compute_chunk(WorkerItem* item) const noexcept
{
    char* opaque = (char*)SDL_calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
    char* light = (char*)SDL_calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char));
    char* highest = (char*)SDL_calloc(XZ_SIZE * XZ_SIZE, sizeof(char));

    int ox = item->p * BUILD_CHUNK_SIZE - BUILD_CHUNK_SIZE - 1;
    int oy = -1;
    int oz = item->q * BUILD_CHUNK_SIZE - BUILD_CHUNK_SIZE - 1;

    // check for lights
    int has_light = 0;
    for (int a = 0; a < 3; a++)
    {
        for (int b = 0; b < 3; b++)
        {
            Map* map = item->light_maps[a][b];
            if (map && map->size)
            {
                has_light = 1;
            }
        }
    }

    // populate opaque array
    for (int a = 0; a < 3; a++)
    {
        for (int b = 0; b < 3; b++)
        {
            Map* block_map = item->block_maps[a][b];
            if (!block_map)
            {
                continue;
            }
            MAP_FOR_EACH(block_map, ex, ey, ez, ew)
                {
                    int x = ex - ox;
                    int y = ey - oy;
                    int z = ez - oz;
                    int w = ew;
                    // TODO: this should be unnecessary
                    if (x < 0 || y < 0 || z < 0)
                    {
                        continue;
                    }
                    if (x >= XZ_SIZE || y >= Y_SIZE || z >= XZ_SIZE)
                    {
                        continue;
                    }
                    // END TODO
                    opaque[XYZ(x, y, z)] = !is_transparent(w);
                    if (opaque[XYZ(x, y, z)])
                    {
                        highest[XZ(x, z)] = SDL_max(highest[XZ(x, z)], y);
                    }
                }
            END_MAP_FOR_EACH;
        }
    }

    // flood fill light intensities
    if (has_light)
    {
        for (int a = 0; a < 3; a++)
        {
            for (int b = 0; b < 3; b++)
            {
                Map* map = item->light_maps[a][b];
                if (!map)
                {
                    continue;
                }
                MAP_FOR_EACH(map, ex, ey, ez, ew)
                    {
                        int x = ex - ox;
                        int y = ey - oy;
                        int z = ez - oz;
                        light_fill(opaque, light, x, y, z, ew, 1);
                    }
                END_MAP_FOR_EACH;
            }
        }
    }

    Map* block_map = item->block_maps[1][1];

    // count exposed faces
    int miny = 256;
    int maxy = 0;
    int faces = 0;
    MAP_FOR_EACH(block_map, ex, ey, ez, ew)
        {
            if (ew <= 0)
            {
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
            if (total == 0)
            {
                continue;
            }
            if (is_plant(ew))
            {
                total = 4;
            }
            miny = SDL_min(miny, ey);
            maxy = SDL_max(maxy, ey);
            faces += total;
        }
    END_MAP_FOR_EACH;

    // generate geometry
    // each vertex has 10 components (x, y, z, nx, ny, nz, u, v, ao, light)
    static constexpr int components = 10;
    GLfloat* data = malloc_faces(components, faces);
    int offset = 0;
    MAP_FOR_EACH(block_map, ex, ey, ez, ew)
        {
            if (ew <= 0)
            {
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
            if (total == 0)
            {
                continue;
            }
            char neighbors[27] = {0};
            char lights[27] = {0};
            float shades[27] = {0};
            int index = 0;
            for (int dx = -1; dx <= 1; dx++)
            {
                for (int dy = -1; dy <= 1; dy++)
                {
                    for (int dz = -1; dz <= 1; dz++)
                    {
                        neighbors[index] = opaque[XYZ(x + dx, y + dy, z + dz)];
                        lights[index] = light[XYZ(x + dx, y + dy, z + dz)];
                        shades[index] = 0;
                        int highest_index = XZ(x + dx, z + dz);
                        if (highest_index < XZ_SIZE * XZ_SIZE && y + dy <= highest[highest_index])
                        {
                            for (int oy = 0; oy < 8; oy++)
                            {
                                if (opaque[XYZ(x + dx, y + dy + oy, z + dz)])
                                {
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
            if (is_plant(ew))
            {
                total = 4;
                float min_ao = 1;
                float max_light = 0;
                for (int a = 0; a < 6; a++)
                {
                    for (int b = 0; b < 4; b++)
                    {
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
            else
            {
                make_cube(
                    data + offset, ao, light,
                    f1, f2, f3, f4, f5, f6,
                    static_cast<float>(ex), static_cast<float>(ey), static_cast<float>(ez), 0.5f, ew);
            }
            offset += total * 60;
        }
    END_MAP_FOR_EACH;

    SDL_free(opaque);
    SDL_free(light);
    SDL_free(highest);

    item->miny = miny;
    item->maxy = maxy;
    item->faces = faces;
    item->data = data;
} // compute_chunk

void world::generate_chunk(Chunk* chunk, WorkerItem* item) const noexcept
{
    chunk->miny = item->miny;
    chunk->maxy = item->maxy;
    chunk->faces = item->faces;
    this->del_buffer(chunk->buffer);
    chunk->buffer = this->gen_faces(10, item->faces, item->data);
    this->gen_sign_buffer(chunk);
}

void world::gen_chunk_buffer(Chunk* chunk) const noexcept
{
    WorkerItem _item;
    WorkerItem* item = &_item;
    item->p = chunk->p;
    item->q = chunk->q;
    for (int dp = -1; dp <= 1; dp++)
    {
        for (int dq = -1; dq <= 1; dq++)
        {
            Chunk* other = chunk;
            if (dp || dq)
            {
                auto other_opt = find_chunk(chunk->p + dp, chunk->q + dq);
                if (!other_opt.has_value())
                {
                    other = nullptr;
                }
                else
                {
                    other = other_opt.value();
                }
            }
            if (other)
            {
                item->block_maps[dp + 1][dq + 1] = &other->map;
                item->light_maps[dp + 1][dq + 1] = &other->lights;
            }
            else
            {
                item->block_maps[dp + 1][dq + 1] = 0;
                item->light_maps[dp + 1][dq + 1] = 0;
            }
        }
    }
    this->compute_chunk(item);
    this->generate_chunk(chunk, item);
    chunk->dirty = 0;
}

void world::map_set_func(int x, int y, int z, int w, Map* m) noexcept
{
    map_set(m, x, y, z, w);
}

// Create a chunk that represents a unique portion of the world
// p, q represents the chunk key
void world::load_chunk(WorkerItem* item) const noexcept
{
    int p = item->p;
    int q = item->q;

    Map* block_map = item->block_maps[1][1];
    Map* light_map = item->light_maps[1][1];

    create_world(p, q, map_set_func, block_map, BUILD_CHUNK_SIZE);
    db_load_blocks(block_map, p, q);
    db_load_lights(light_map, p, q);
}

void world::init_chunk(Chunk* chunk, int p, int q) const noexcept
{
    chunk->p = p;
    chunk->q = q;
    chunk->faces = 0;
    chunk->sign_faces = 0;
    chunk->buffer = 0;
    chunk->sign_buffer = 0;
    dirty_chunk(chunk);
    auto* signs = &chunk->signs;
    sign_list_alloc(signs, 16);
    db_load_signs(signs, p, q);
    Map* block_map = &chunk->map;
    Map* light_map = &chunk->lights;
    int dx = p * BUILD_CHUNK_SIZE - 1;
    int dy = 0;
    int dz = q * BUILD_CHUNK_SIZE - 1;
    map_alloc(block_map, dx, dy, dz, 0x7fff);
    map_alloc(light_map, dx, dy, dz, 0xf);
}

void world::create_chunk(Chunk* chunk, int p, int q) const noexcept
{
    init_chunk(chunk, p, q);

    WorkerItem _item;
    WorkerItem* item = &_item;
    item->p = chunk->p;
    item->q = chunk->q;
    item->block_maps[1][1] = &chunk->map;
    item->light_maps[1][1] = &chunk->lights;

    load_chunk(item);
}

void world::delete_chunks() noexcept
{
    int count = this->m_model.chunk_count;
    const player::state* s1 = &m_player->s1;
    for (int i = 0; i < count; i++)
    {
        Chunk* chunk = this->m_model.chunks + i;
        int remove_chunk = 1;
        int p = chunked(s1->x);
        int q = chunked(s1->z);
        if (chunk_distance(chunk, p, q) < this->m_model.delete_radius)
        {
            remove_chunk = 0;
            break;
        }
        if (remove_chunk)
        {
            map_free(&chunk->map);
            map_free(&chunk->lights);
            sign_list_free(reinterpret_cast<::SignList*>(&chunk->signs));
            del_buffer(chunk->buffer);
            del_buffer(chunk->sign_buffer);
            Chunk* other = this->m_model.chunks + (--count);
            SDL_memcpy(chunk, other, sizeof(Chunk));
        }
    }
    this->m_model.chunk_count = count;
}

void world::delete_all_chunks() noexcept
{
    for (int i = 0; i < this->m_model.chunk_count; i++)
    {
        Chunk* chunk = this->m_model.chunks + i;
        map_free(&chunk->map);
        map_free(&chunk->lights);
        sign_list_free(reinterpret_cast<::SignList*>(&chunk->signs));
        del_buffer(chunk->buffer);
        del_buffer(chunk->sign_buffer);
    }
    this->m_model.chunk_count = 0;
}

void world::check_workers() noexcept
{
    for (auto&& worker : this->m_model.workers)
    {
        worker->mtx.lock();
        if (worker->state == WORKER_DONE)
        {
            WorkerItem* item = &worker->item;
            auto chunk_opt = find_chunk(item->p, item->q);
            if (chunk_opt.has_value())
            {
                Chunk* chunk = chunk_opt.value();
                if (item->load)
                {
                    Map* block_map = item->block_maps[1][1];
                    Map* light_map = item->light_maps[1][1];
                    map_free(&chunk->map);
                    map_free(&chunk->lights);
                    map_copy(&chunk->map, block_map);
                    map_copy(&chunk->lights, light_map);
                }
                generate_chunk(chunk, item);
            }
            for (int a = 0; a < 3; a++)
            {
                for (int b = 0; b < 3; b++)
                {
                    Map* block_map = item->block_maps[a][b];
                    Map* light_map = item->light_maps[a][b];
                    if (block_map)
                    {
                        map_free(block_map);
                    }
                    if (light_map)
                    {
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
void world::force_chunks(player* _player) noexcept
{
    player::state* s = &_player->s1;
    int p = chunked(s->x);
    int q = chunked(s->z);

    int r = 1;
    for (int dp = -r; dp <= r; dp++)
    {
        for (int dq = -r; dq <= r; dq++)
        {
            int a = p + dp;
            int b = q + dq;
            auto chunk_opt = find_chunk(a, b);
            if (chunk_opt.has_value())
            {
                Chunk* chunk = chunk_opt.value();
                if (chunk->dirty)
                {
                    gen_chunk_buffer(chunk);
                }
            }
            else if (this->m_model.chunk_count < MAX_CHUNKS)
            {
                Chunk* chunk = this->m_model.chunks + this->m_model.chunk_count++;
                create_chunk(chunk, a, b);
                gen_chunk_buffer(chunk);
            }
        }
    }
}

void world::ensure_chunks_worker(player* _player, Worker* worker) noexcept
{
    player::state* s = &_player->s1;
    float matrix[16];
    set_matrix_3d(matrix, m_model.voxel_scene_w, m_model.voxel_scene_h,
                  s->x, s->y, s->z, s->rx, s->ry, m_model.fov, m_model.is_ortho, m_model.render_radius);
    float planes[6][4];
    frustum_planes(planes, this->m_model.render_radius, matrix);
    int p = chunked(s->x);
    int q = chunked(s->z);
    // int r = this->m_model.create_radius;
    int r{this->m_model.create_radius};
    int start = 0x0fffffff;
    int best_score = start;
    int best_a = 0;
    int best_b = 0;
    for (int dp = -r; dp <= r; dp++)
    {
        for (int dq = -r; dq <= r; dq++)
        {
            int a = p + dp;
            int b = q + dq;
            int index = (SDL_abs(a) ^ SDL_abs(b)) % NUM_WORKERS;
            if (index != worker->index)
            {
                continue;
            }
            auto chunk_opt = find_chunk(a, b);
            if (chunk_opt.has_value() && !chunk_opt.value()->dirty)
            {
                continue;
            }
            int distance = SDL_max(SDL_abs(dp), SDL_abs(dq));
            int invisible = ~chunk_visible(planes, a, b, 0, 256);
            int priority = 0;
            if (chunk_opt.has_value())
            {
                Chunk* chunk = chunk_opt.value();
                priority = chunk->buffer & chunk->dirty;
            }
            // Check for chunk to update based on lowest score
            int score = (invisible << 24) | (priority << 16) | distance;
            if (score < best_score)
            {
                best_score = score;
                best_a = a;
                best_b = b;
            }
        }
    }
    if (best_score == start)
    {
        return;
    }
    int a = best_a;
    int b = best_b;
    int load = 0;
    auto chunk_opt = find_chunk(a, b);
    Chunk* chunk = nullptr;
    // Check if the chunk is already loaded
    if (!chunk_opt.has_value())
    {
        load = 1;
        if (this->m_model.chunk_count < MAX_CHUNKS)
        {
            chunk = this->m_model.chunks + this->m_model.chunk_count++;
            init_chunk(chunk, a, b);
        }
        else
        {
            return;
        }
    }
    else
    {
        chunk = chunk_opt.value();
    }
    WorkerItem* item = &worker->item;
    item->p = chunk->p;
    item->q = chunk->q;
    item->load = load;
    for (int dp = -1; dp <= 1; dp++)
    {
        for (int dq = -1; dq <= 1; dq++)
        {
            Chunk* other = chunk;
            if (dp || dq)
            {
                auto other_opt = find_chunk(chunk->p + dp, chunk->q + dq);
                if (!other_opt.has_value())
                {
                    other = nullptr;
                }
                else
                {
                    other = other_opt.value();
                }
            }
            if (other)
            {
                // These maps are freed using C-library free function
                Map* block_map = (Map*)malloc(sizeof(Map));
                map_copy(block_map, &other->map);
                Map* light_map = (Map*)malloc(sizeof(Map));
                map_copy(light_map, &other->lights);
                item->block_maps[dp + 1][dq + 1] = block_map;
                item->light_maps[dp + 1][dq + 1] = light_map;
            }
            else
            {
                item->block_maps[dp + 1][dq + 1] = 0;
                item->light_maps[dp + 1][dq + 1] = 0;
            }
        }
    }
    chunk->dirty = 0;
    worker->state = WORKER_BUSY;
    worker->cnd.notify_one();
} // ensure chunks worker

void world::ensure_chunks(player* _player) noexcept
{
    check_workers();
    force_chunks(_player);
    for (auto&& worker : m_model.workers)
    {
        worker->mtx.lock();
        if (worker->state == WORKER_IDLE)
        {
            ensure_chunks_worker(_player, worker.get());
        }
        worker->mtx.unlock();
    }
}

void world::unset_sign(int x, int y, int z) const noexcept
{
    int p = chunked(static_cast<float>(x));
    int q = chunked(static_cast<float>(z));
    auto chunk_opt = find_chunk(p, q);
    if (chunk_opt.has_value())
    {
        Chunk* chunk = chunk_opt.value();
        ::SignList* signs = reinterpret_cast<::SignList*>(&chunk->signs);
        if (sign_list_remove_all(signs, x, y, z))
        {
            chunk->dirty = 1;
            db_delete_signs(x, y, z);
        }
    }
    else
    {
        db_delete_signs(x, y, z);
    }
}

void world::unset_sign_face(int x, int y, int z, int face) const noexcept
{
    int p = chunked(static_cast<float>(x));
    int q = chunked(static_cast<float>(z));
    auto chunk_opt = find_chunk(p, q);
    if (chunk_opt.has_value())
    {
        Chunk* chunk = chunk_opt.value();
        ::SignList* signs = reinterpret_cast<::SignList*>(&chunk->signs);
        if (sign_list_remove(signs, x, y, z, face))
        {
            chunk->dirty = 1;
            db_delete_sign(x, y, z, face);
        }
    }
    else
    {
        db_delete_sign(x, y, z, face);
    }
}

void world::_set_sign(int p, int q, int x, int y, int z, int face, std::string_view text, int dirty) const noexcept
{
    if (text.length() == 0)
    {
        unset_sign_face(x, y, z, face);
        return;
    }
    auto chunk_opt = find_chunk(p, q);
    if (chunk_opt.has_value())
    {
        Chunk* chunk = chunk_opt.value();
        ::SignList* signs = reinterpret_cast<::SignList*>(&chunk->signs);
        sign_list_add(signs, x, y, z, face, text.data());
        if (dirty)
        {
            chunk->dirty = 1;
        }
    }
    db_insert_sign(p, q, x, y, z, face, text.data());
}

void world::set_sign(int x, int y, int z, int face, std::string_view text) const noexcept
{
    int p = chunked(static_cast<float>(x));
    int q = chunked(static_cast<float>(z));
    _set_sign(p, q, x, y, z, face, text, 1);
}

void world::toggle_light(int x, int y, int z) const noexcept
{
    int p = chunked(static_cast<float>(x));
    int q = chunked(static_cast<float>(z));
    auto chunk_opt = find_chunk(p, q);
    if (chunk_opt.has_value())
    {
        Chunk* chunk = chunk_opt.value();
        Map* map = &chunk->lights;
        int w = map_get(map, x, y, z) ? 0 : 15;
        map_set(map, x, y, z, w);
        db_insert_light(p, q, x, y, z, w);
        dirty_chunk(chunk);
    }
}

void world::set_light(int p, int q, int x, int y, int z, int w) const noexcept
{
    auto chunk_opt = find_chunk(p, q);
    if (chunk_opt.has_value())
    {
        Chunk* chunk = chunk_opt.value();
        Map* map = &chunk->lights;
        if (map_set(map, x, y, z, w))
        {
            dirty_chunk(chunk);
            db_insert_light(p, q, x, y, z, w);
        }
    }
    else
    {
        db_insert_light(p, q, x, y, z, w);
    }
}

void world::_set_block(int p, int q, int x, int y, int z, int w, int dirty) const noexcept
{
    auto chunk_opt = find_chunk(p, q);
    if (chunk_opt.has_value())
    {
        Chunk* chunk = chunk_opt.value();
        Map* map = &chunk->map;
        if (map_set(map, x, y, z, w))
        {
            if (dirty)
            {
                dirty_chunk(chunk);
            }
            db_insert_block(p, q, x, y, z, w);
        }
    }
    else
    {
        db_insert_block(p, q, x, y, z, w);
    }
    if (w == 0 && chunked(static_cast<float>(x)) == p && chunked(static_cast<float>(z)) == q)
    {
        unset_sign(x, y, z);
        set_light(p, q, x, y, z, 0);
    }
}

void world::set_block(int x, int y, int z, int w) const noexcept
{
    int p = chunked(static_cast<float>(x));
    int q = chunked(static_cast<float>(z));
    _set_block(p, q, x, y, z, w, 1);
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dz = -1; dz <= 1; dz++)
        {
            if (dx == 0 && dz == 0)
            {
                continue;
            }
            if (dx && chunked(static_cast<float>(x + dx)) == p)
            {
                continue;
            }
            if (dz && chunked(static_cast<float>(z + dz)) == q)
            {
                continue;
            }
            _set_block(p + dx, q + dz, x, y, z, -w, 1);
        }
    }
}

void world::record_block(int x, int y, int z, int w) noexcept
{
    SDL_memcpy(&this->m_model.block1, &this->m_model.block0, sizeof(Block));
    this->m_model.block0.x = x;
    this->m_model.block0.y = y;
    this->m_model.block0.z = z;
    this->m_model.block0.w = w;
}

int world::get_block(int x, int y, int z) noexcept
{
    int p = chunked(static_cast<float>(x));
    int q = chunked(static_cast<float>(z));
    auto chunk_opt = find_chunk(p, q);
    if (chunk_opt.has_value())
    {
        Chunk* chunk = chunk_opt.value();
        Map* map = &chunk->map;
        return map_get(map, x, y, z);
    }
    return 0;
}

void world::builder_block(int x, int y, int z, int w) noexcept
{
    if (y <= 0 || y >= 256)
    {
        return;
    }
    if (is_destructable(get_block(x, y, z)))
    {
        set_block(x, y, z, 0);
    }
    if (w)
    {
        set_block(x, y, z, w);
    }
}

int world::render_chunks(const Attrib* attrib, player* _player, std::uint32_t texture) const noexcept
{
    int result = 0;
    player::state* s = &_player->s1;
    // ensure_chunks(_player);
    int p = this->chunked(s->x);
    int q = this->chunked(s->z);
    float light = this->get_daylight();
    float matrix[16];
    // matrix.cpp -> set_matrix_3d
    set_matrix_3d(
        matrix, this->m_model.voxel_scene_w, this->m_model.voxel_scene_h,
        s->x, s->y, s->z, s->rx, s->ry, this->m_model.fov, static_cast<int>(this->m_model.is_ortho),
        this->m_model.render_radius);

    // Debug: Log matrix and view parameters periodically
    static int render_frame = 0;
    if (render_frame++ % 120 == 0) {
        SDL_Log("render_chunks: viewport=%dx%d, pos=(%.2f,%.2f,%.2f), rot=(%.2f,%.2f), fov=%.1f, chunks=%d",
                this->m_model.voxel_scene_w, this->m_model.voxel_scene_h,
                s->x, s->y, s->z, s->rx, s->ry, this->m_model.fov, this->m_model.chunk_count);
    }

    float planes[6][4];
    // matrix.cpp -> frustum_planes
    frustum_planes(planes, this->m_model.render_radius, matrix);
    glUseProgram(attrib->program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform3f(attrib->camera, s->x, s->y, s->z);
    glUniform1i(attrib->sampler, 0);
    glUniform1f(attrib->extra2, light);
    glUniform1f(attrib->extra3, static_cast<GLfloat>(this->m_model.render_radius * BUILD_CHUNK_SIZE));
    glUniform1i(attrib->extra4, static_cast<int>(this->m_model.is_ortho));
    glUniform1f(attrib->timer, this->time_of_day());
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);

    int chunks_rendered = 0;
    int chunks_culled_distance = 0;
    int chunks_culled_frustum = 0;

    for (int i = 0; i < this->m_model.chunk_count; i++)
    {
        const Chunk* chunk = this->m_model.chunks + i;
        if (chunk_distance(chunk, p, q) > this->m_model.render_radius)
        {
            chunks_culled_distance++;
            continue;
        }
        if (!chunk_visible(planes, chunk->p, chunk->q, chunk->miny, chunk->maxy))
        {
            chunks_culled_frustum++;
            continue;
        }
        this->draw_chunk(attrib, chunk);
        result += chunk->faces;
        chunks_rendered++;
    }

    // Debug: Log culling statistics
    if (render_frame % 120 == 0) {
        SDL_Log("Chunk stats: total=%d, rendered=%d, culled_distance=%d, culled_frustum=%d",
                this->m_model.chunk_count, chunks_rendered, chunks_culled_distance, chunks_culled_frustum);
    }

    return result;
}

void world::render_signs(const Attrib* attrib, player* _player, std::uint32_t sign) const noexcept
{
    player::state* s = &_player->s1;
    int p = chunked(s->x);
    int q = chunked(s->z);
    float matrix[16];
    set_matrix_3d(
        matrix, this->m_model.voxel_scene_w, this->m_model.voxel_scene_h,
        s->x, s->y, s->z, s->rx, s->ry, this->m_model.fov, static_cast<int>(this->m_model.is_ortho),
        this->m_model.render_radius);
    float planes[6][4];
    frustum_planes(planes, this->m_model.render_radius, matrix);

    glUseProgram(attrib->program);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, sign);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform1i(attrib->sampler, 2);
    glUniform1i(attrib->extra1, 1);

    for (int i = 0; i < this->m_model.chunk_count; i++)
    {
        const Chunk* chunk = this->m_model.chunks + i;
        if (chunk_distance(chunk, p, q) > this->m_model.sign_radius)
        {
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

void world::render_sign(const Attrib* attrib, player* _player, const std::uint32_t sign) const noexcept
{
    int x, y, z, face;
    if (!hit_test_face(_player, &x, &y, &z, &face))
    {
        return;
    }

    player::state* s = &_player->s1;
    float matrix[16];
    set_matrix_3d(
        matrix, this->m_model.voxel_scene_w, this->m_model.voxel_scene_h,
        s->x, s->y, s->z, s->rx, s->ry, this->m_model.fov, static_cast<int>(this->m_model.is_ortho),
        this->m_model.render_radius);
    glUseProgram(attrib->program);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, sign);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform1i(attrib->sampler, 2);
    glUniform1i(attrib->extra1, 1);
    char text[MAX_SIGN_LENGTH];
    SDL_strlcpy(text, "put maze here", MAX_SIGN_LENGTH);
    text[MAX_SIGN_LENGTH - 1] = '\0';
    GLfloat* data = malloc_faces(5, SDL_strlen(text));
    int length = _gen_sign_buffer(data, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), face,
                                  text);
    GLuint buffer = gen_faces(5, length, data);
    draw_sign(attrib, buffer, length);
    del_buffer(buffer);
}

void world::render_players(const Attrib* attrib, player* _player) const noexcept
{
    player::state* s = &_player->s1;
    float matrix[16];
    set_matrix_3d(
        matrix, this->m_model.voxel_scene_w, this->m_model.voxel_scene_h,
        s->x, s->y, s->z, s->rx, s->ry, this->m_model.fov, static_cast<int>(this->m_model.is_ortho),
        this->m_model.render_radius);
    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform3f(attrib->camera, s->x, s->y, s->z);
    glUniform1i(attrib->sampler, 0);
    glUniform1f(attrib->timer, time_of_day());
    for (int i = 0; i < this->m_model.player_count; i++)
    {
        const player* other = m_player;
        draw_player(attrib, other);
    }
}

void world::render_wireframe(const Attrib* attrib, player* _player) const noexcept
{
    player::state* s = &_player->s1;
    float matrix[16];
    set_matrix_3d(
        matrix, this->m_model.voxel_scene_w, this->m_model.voxel_scene_h,
        s->x, s->y, s->z, s->rx, s->ry, this->m_model.fov, static_cast<int>(this->m_model.is_ortho),
        this->m_model.render_radius);
    int hx, hy, hz;
    const int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    if (is_obstacle(hw))
    {
        glUseProgram(attrib->program);
        glLineWidth(1);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        GLuint wireframe_buffer = gen_wireframe_buffer(static_cast<float>(hx), static_cast<float>(hy),
                                                       static_cast<float>(hz), 0.53f);
        draw_lines(attrib, wireframe_buffer, 3, 24);
        del_buffer(wireframe_buffer);
    }
}

void world::render_crosshairs(const Attrib* attrib) const noexcept
{
    float matrix[16];
    set_matrix_2d(matrix, this->m_model.voxel_scene_w, this->m_model.voxel_scene_h);
    glUseProgram(attrib->program);
    glLineWidth(static_cast<GLfloat>(4 * this->m_model.scale));
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    GLuint crosshair_buffer = gen_crosshair_buffer();
    draw_lines(attrib, crosshair_buffer, 2, 4);
    del_buffer(crosshair_buffer);
}

void world::render_item(const Attrib* attrib, std::uint32_t texture) const noexcept
{
    float matrix[16];
    set_matrix_item(matrix, this->m_model.voxel_scene_w, this->m_model.voxel_scene_h, this->m_model.scale);
    glUseProgram(attrib->program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform3f(attrib->camera, 0, 0, 5);
    glUniform1i(attrib->sampler, 0);
    glUniform1f(attrib->timer, time_of_day());
    int w = items[this->m_model.item_index];
    if (is_plant(w))
    {
        GLuint buffer = gen_plant_buffer(0, 0, 0, 0.5, w);
        draw_plant(attrib, buffer);
        del_buffer(buffer);
    }
    else
    {
        GLuint buffer = gen_cube_buffer(0, 0, 0, 0.5, w);
        draw_cube(attrib, buffer);
        del_buffer(buffer);
    }
}

void world::render_text(const Attrib* attrib, std::uint32_t font,
                        int justify, float x, float y, float n, std::string_view text) const noexcept
{
    float matrix[16];
    set_matrix_2d(matrix, this->m_model.voxel_scene_w, this->m_model.voxel_scene_h);
    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform1i(attrib->sampler, 3);
    glUniform1i(attrib->extra1, 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, font);
    GLsizei length = static_cast<GLsizei>(text.length());
    x -= n * justify * (length - 1) / 2;
    GLuint buffer = gen_text_buffer(x, y, n, text);
    draw_text(attrib, buffer, length);
    del_buffer(buffer);
}

void world::on_light() noexcept
{
    player::state* s = &this->m_player->s1;
    int hx, hy, hz;
    if (const int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        hy > 0 && hy < 256 && is_destructable(hw))
    {
        toggle_light(hx, hy, hz);
    }
}

void world::on_left_click() noexcept
{
    const player::state* s = &this->m_player->s1;
    int hx, hy, hz;
    if (const auto hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        hy > 0 && hy < 256 && is_destructable(hw))
    {
        set_block(hx, hy, hz, 0);
        record_block(hx, hy, hz, 0);
#if defined(MAZE_DEBUG)
        SDL_Log("on_left_click(%d, %d, %d, %d, block_type: %d): ", hx, hy, hz, hw, items[this->m_model.item_index]);
#endif
        if (is_plant(get_block(hx, hy + 1, hz)))
        {
            set_block(hx, hy + 1, hz, 0);
        }
    }
}

void world::on_right_click() noexcept
{
    const player::state* s = &this->m_player->s1;
    int hx, hy, hz;
    int hw = hit_test(1, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    if (hy > 0 && hy < 256 && is_obstacle(hw))
    {
        if (!player_intersects_block(2, s->x, s->y, s->z, hx, hy, hz))
        {
            set_block(hx, hy, hz, items[this->m_model.item_index]);
            record_block(hx, hy, hz, items[this->m_model.item_index]);
#if defined(MAZE_DEBUG)
            SDL_Log("on_right_click(%d, %d, %d, %d, block_type: %d): ", hx, hy, hz, hw,
                    items[this->m_model.item_index]);
#endif
        }
    }
}

void world::on_middle_click() noexcept
{
    const player::state* s = &this->m_player->s1;
    int hx, hy, hz;
    const int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    for (int i = 0; i < item_count; i++)
    {
        if (items[i] == hw)
        {
            this->m_model.item_index = i;
#if defined(MAZE_DEBUG)
            SDL_Log("Copying item index: %d\n", i);
#endif
            break;
        }
    }
}

