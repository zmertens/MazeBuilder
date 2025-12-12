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
#include "scene_node.h"
#include "sdl_gl_helper.h"
#include "shader.h"
#include "sign.h"

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>

#include <algorithm>
#include <ranges>

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
#define DAY_LENGTH 600
#define MAX_TEXT_LENGTH 256
#define CREATE_CHUNK_RADIUS 10
#define RENDER_CHUNK_RADIUS 20
#define BUILD_CHUNK_SIZE 32
#define RENDER_SIGN_RADIUS 4
#define DELETE_CHUNK_RADIUS 14
#define MAX_PLAYERS 1
#define NUM_WORKERS 4

static sdl_gl_helper::attrib s_block_attrib, s_line_attrib, s_text_attrib, s_sky_attrib;

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
        texture_manager& textures,
        const sdl_gl_helper* sdl)
    : m_sdl{sdl}
      , m_fonts{fonts}
      , m_shaders{shaders}
      , m_textures{textures}
      , m_scene_layers{}
      , m_next_chunk_slot{1}  // Start at 1 since index 0 is reserved for root layer node
      , m_command_queue{}
      , m_player{p}
      , m_model{}
{
    // Set bidirectional reference between player and world
    if (m_player)
    {
        m_player->set_world(this);
    }
}

world::~world()
{
    destroy_world();
}

void world::build_scene()
{
    // Initialize the BACKGROUND layer with a root node at index 0
    auto& background_layer = m_scene_layers[static_cast<std::size_t>(Layer::BACKGROUND)];

    // Create root layer node at index 0
    scene_node* root = new scene_node{};
    root->set_category(Entity::SCENE);
    root->bounds_min_x = -10000;
    root->bounds_min_z = -10000;
    root->bounds_max_x = 10000;
    root->bounds_max_z = 10000;
    root->parent = nullptr;
    background_layer[0] = root;

    // Initialize remaining slots to nullptr
    for (std::size_t i = 1; i < background_layer.size(); ++i)
    {
        background_layer[i] = nullptr;
    }

    // Initialize FOREGROUND layer similarly if needed in the future
    auto& foreground_layer = m_scene_layers[static_cast<std::size_t>(Layer::FOREGROUND)];
    for (std::size_t i = 0; i < foreground_layer.size(); ++i)
    {
        foreground_layer[i] = nullptr;
    }
}

void world::attach_chunk_to_layer(scene_node* chunk, int layer_index) noexcept
{
    if (layer_index >= static_cast<int>(Layer::LAYER_COUNT) || chunk == nullptr)
    {
        return;
    }

    // Set chunk bounds based on its p,q coordinates
    constexpr int CHUNK_SIZE = BUILD_CHUNK_SIZE;
    chunk->bounds_min_x = chunk->p * CHUNK_SIZE;
    chunk->bounds_min_z = chunk->q * CHUNK_SIZE;
    chunk->bounds_max_x = (chunk->p + 1) * CHUNK_SIZE - 1;
    chunk->bounds_max_z = (chunk->q + 1) * CHUNK_SIZE - 1;
    chunk->set_category(Entity::CHUNK);

    // Insert into spatial hierarchy
    insert_chunk_into_spatial_tree(chunk);
}

void world::detach_chunk_from_layer(scene_node* chunk) noexcept
{
    if (chunk == nullptr || chunk->parent == nullptr)
    {
        return;
    }

    // Remove from spatial hierarchy
    remove_chunk_from_spatial_tree(chunk);
}

void world::insert_chunk_into_spatial_tree(scene_node* chunk) const noexcept
{
    if (chunk == nullptr)
    {
        return;
    }

    // Get the layer root (background layer)
    scene_node* root = m_scene_layers[static_cast<std::size_t>(Layer::BACKGROUND)].at(0);
    if (root == nullptr)
    {
        return;
    }

    // For now, use a simple spatial subdivision approach
    // Find or create an appropriate spatial node to hold this chunk

    if (scene_node* spatial_parent = root; spatial_parent != nullptr)
    {
        chunk->parent = spatial_parent;

        if (const bool is_attached = std::ranges::find(spatial_parent->children, chunk) != spatial_parent->children.cend();
            !is_attached)
        {
            spatial_parent->children.push_back(chunk);
        }
    }
}

void world::remove_chunk_from_spatial_tree(scene_node* chunk) noexcept
{
    if (chunk == nullptr || chunk->parent == nullptr)
    {
        return;
    }

    // Remove from parent's children list
    auto& siblings = chunk->parent->children;
    siblings.erase(std::remove(siblings.begin(), siblings.end(), chunk), siblings.end());

    chunk->parent = nullptr;
}

void world::traverse_chunks(const std::function<void(scene_node*)>& callback) const noexcept
{
    // Traverse the spatial hierarchy
    scene_node* root = m_scene_layers[static_cast<std::size_t>(Layer::BACKGROUND)].at(0);
    if (root == nullptr)
    {
        return;
    }

    // Recursive traversal helper
    std::function<void(scene_node*)> traverse_node = [&](scene_node* node)
    {
        if (node == nullptr)
        {
            return;
        }

        // If this is a chunk, call the callback
        if (node->get_category() == Entity::CHUNK)
        {
            callback(node);
        }

        // Traverse children
        for (scene_node* child : node->children)
        {
            traverse_node(child);
        }
    };

    traverse_node(root);
}

void world::traverse_chunks_in_bounds(int min_p, int min_q, int max_p, int max_q,
                                       const std::function<void(scene_node*)>& callback) const noexcept
{
    // Convert chunk coordinates to world coordinates
    constexpr int CHUNK_SIZE = 32;
    int min_x = min_p * CHUNK_SIZE;
    int min_z = min_q * CHUNK_SIZE;
    int max_x = (max_p + 1) * CHUNK_SIZE - 1;
    int max_z = (max_q + 1) * CHUNK_SIZE - 1;

    scene_node* root = m_scene_layers[static_cast<std::size_t>(Layer::BACKGROUND)].at(0);
    if (root == nullptr)
    {
        return;
    }

    // Recursive traversal with bounds checking
    std::function<void(scene_node*)> traverse_node = [&](scene_node* node)
    {
        if (node == nullptr)
        {
            return;
        }

        // Early rejection: if node doesn't intersect bounds, skip it and all children
        if (!node->intersects_bounds(min_x, min_z, max_x, max_z))
        {
            return;
        }

        // If this is a chunk and it intersects, call the callback
        if (node->get_category() == Entity::CHUNK)
        {
            callback(node);
        }

        // Traverse children (only if this node intersects)
        for (scene_node* child : node->children)
        {
            traverse_node(child);
        }
    };

    traverse_node(root);
}

void world::init() noexcept
{
    m_model.sign_radius = RENDER_SIGN_RADIUS;
    m_model.flying = false;
    m_model.item_index = 0;
    m_model.is_ortho = false;
    m_model.fov = 65.0f;
    m_model.day_length = DAY_LENGTH;

    // Set up OpenGL state (critical for rendering)
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
#if !defined(__EMSCRIPTEN__)

    glLogicOp(GL_INVERT);
#endif

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Build scene graph and initialize chunks
    build_scene();

    init_worker_threads();

    // Force create initial chunks around player
    force_chunks(m_player);

    // Set player Y position to proper height above terrain
    m_player->s1.y = static_cast<float>(highest_block(m_player->s1.x, m_player->s1.z) + 2);

    s_block_attrib.program = m_shaders.get(ShaderIdentifier::BLOCK_SHADER).get();
    s_block_attrib.position = 0;
    s_block_attrib.normal = 1;
    s_block_attrib.uv = 2;
    s_block_attrib.matrix = glGetUniformLocation(s_block_attrib.program, "matrix");
    s_block_attrib.sampler = glGetUniformLocation(s_block_attrib.program, "sampler");
    s_block_attrib.extra1 = glGetUniformLocation(s_block_attrib.program, "sky_sampler");
    s_block_attrib.extra2 = glGetUniformLocation(s_block_attrib.program, "daylight");
    s_block_attrib.extra3 = glGetUniformLocation(s_block_attrib.program, "fog_distance");
    s_block_attrib.extra4 = glGetUniformLocation(s_block_attrib.program, "is_ortho");
    s_block_attrib.camera = glGetUniformLocation(s_block_attrib.program, "camera");
    s_block_attrib.timer = glGetUniformLocation(s_block_attrib.program, "timer");

    s_line_attrib.program = m_shaders.get(ShaderIdentifier::LINE_SHADER).get();
    s_line_attrib.position = 0;
    s_line_attrib.matrix = glGetUniformLocation(s_line_attrib.program, "matrix");

    s_text_attrib.program = m_shaders.get(ShaderIdentifier::TEXT_SHADER).get();
    s_text_attrib.position = 0;
    s_text_attrib.uv = 1;
    s_text_attrib.matrix = glGetUniformLocation(s_text_attrib.program, "matrix");
    s_text_attrib.sampler = glGetUniformLocation(s_text_attrib.program, "sampler");
    s_text_attrib.extra1 = glGetUniformLocation(s_text_attrib.program, "is_sign");
}

void world::update(float delta_time, mazes::randomizer& rng) noexcept
{
    m_model.scale = m_sdl->get_scale_factor();

    // Process all commands in the queue
    static int update_frame = 0;
    int commands_processed = 0;

    while (!m_command_queue.is_empty())
    {
        command cmd = m_command_queue.pop();
        cmd.action(*m_player, delta_time);
        commands_processed++;
    }

    update_frame++;

    // Apply gravity as continuous force (convert delta_time from ms to seconds)
    const float dt_seconds = delta_time / 1000.0f;

    // Only apply gravity when not flying
    if (!m_player->m_is_flying)
    {
        m_player->vel.vy += FORCE_DUE_TO_GRAVITY * dt_seconds;
    }
    else
    {
        // In flying mode, apply damping to vertical velocity to stop floating
        m_player->vel.vy *= 0.85f;
    }

    // Apply velocity to position
    m_player->s1.y += m_player->vel.vy * dt_seconds;

    // Apply collision detection (height = 2 blocks for player)
    // Skip collision when flying (allows clipping through blocks)
    if (!m_player->m_is_flying)
    {
        const int collision_result = collide(2, &m_player->s1.x, &m_player->s1.y, &m_player->s1.z);

        // Update ground state based on collision via helper (world is friend of player)
        if (collision_result == 1)
        {
            m_player->vel.vy = 0.0f;  // Stop vertical velocity on collision
            m_player->m_on_ground = true;
        }
        else
        {
            m_player->m_on_ground = false;
        }
    }
    else
    {
        // When flying, not on ground
        m_player->m_on_ground = false;
    }

    delete_chunks();
    sdl_gl_helper::del_buffer(m_player->get_buffer());
    ensure_chunks(m_player);

    // OPTIMIZATION: Process dirty chunks asynchronously on worker threads
    // This prevents FPS drops when placing/destroying blocks
    update_dirty_chunks_async();
}

void world::draw() const noexcept
{
    CHECK_GL_ERR();

    // Set viewport to match window dimensions
    int viewport_width, viewport_height;
    SDL_GetWindowSizeInPixels(m_sdl->window, &viewport_width, &viewport_height);
    glViewport(0, 0, viewport_width, viewport_height);

    // Verify OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
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

    auto triangle_faces = render_chunks(&s_block_attrib, m_player, atlas_texture);

    // Debug logging (can be commented out after testing)
    static int frame_count = 0;
    if (frame_count++ % 60 == 0) {
        SDL_Log("Frame %d: Rendered %d triangle faces, player at (%.2f, %.2f, %.2f), rot (%.2f, %.2f)",
                frame_count, triangle_faces,
                m_player->s1.x, m_player->s1.y, m_player->s1.z,
                m_player->s1.rx, m_player->s1.ry);
    }

    render_item(&s_block_attrib, atlas_texture);

    render_signs(&s_text_attrib, m_player, signs_texture);
    render_sign(&s_text_attrib, m_player, signs_texture);

    render_wireframe(&s_line_attrib, m_player);

    render_crosshairs(&s_line_attrib);
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
    sdl_gl_helper::del_buffer(m_player->get_buffer());
}

void world::handle_event(const SDL_Event& event) noexcept
{
    if (event.type == SDL_EVENT_QUIT)
    {
        // Handle quit event if needed
    }
    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        if (event.button.button == SDL_BUTTON_LEFT)
        {
            // Left click - destroy block
            on_left_click();
        }
        else if (event.button.button == SDL_BUTTON_RIGHT)
        {
            // Right click - build block
            on_right_click();
        }
        else if (event.button.button == SDL_BUTTON_MIDDLE)
        {
            // Middle click - copy block
            on_middle_click();
        }
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
            for (int y = 0; y < h; y++)
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

bool world::worker_run(worker* w) const noexcept
{
    while (true)
    {
        while (w->state != WorkerState::BUSY && !w->should_stop)
        {
            std::unique_lock<std::mutex> my_lock(w->mtx);
            w->cnd.wait(my_lock);
        }
        if (w->should_stop)
        {
            break;
        }
        worker_item* worker_item = &w->item;
        if (worker_item->load)
        {
            this->load_chunk(worker_item);
        }

        this->compute_chunk(worker_item);

        w->mtx.lock();
        w->state = WorkerState::DONE;
        w->mtx.unlock();
    }
    return true;
} // worker_run

void world::init_worker_threads() noexcept
{
    m_workers.reserve(NUM_WORKERS);
    for (int i = 0; i < NUM_WORKERS; i++)
    {
        auto w = std::make_unique<worker>();
        w->index = i;
        w->state = WorkerState::IDLE;
        w->should_stop = false;
        m_workers.emplace_back(std::move(w));
        worker* worker_ptr = m_workers.back().get();
        worker_ptr->thrd = std::thread([this, worker_ptr]() { this->worker_run(worker_ptr); });
    }
}

void world::cleanup_worker_threads() noexcept
{
    // signal all worker threads to stop
    for (auto&& w : m_workers)
    {
        w->mtx.lock();
        w->should_stop = true;
        w->cnd.notify_one();
        w->mtx.unlock();
    }

    for (auto&& w : m_workers)
    {
        // Wait for the thread to complete its execution
        w->thrd.join();

        SDL_Log("worker thread %d finished!", w->index);
    }

    m_workers.clear();
}

int world::chunked(const float x) noexcept
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

std::uint32_t world::gen_crosshair_buffer() const noexcept
{
    auto [width, height] = m_sdl->get_window_size();
    const float x = static_cast<float>(width) / 2.0f;
    const float y = static_cast<float>(height) / 2.0f;
    const float p = 10.f * static_cast<float>(m_model.scale);
    const float data[] = {
        x, y - p, x, y + p,
        x - p, y, x + p, y
    };
    return sdl_gl_helper::gen_buffer(sizeof(data), data);
}

std::uint32_t world::gen_wireframe_buffer(float x, float y, float z, float n) const noexcept
{
    float data[72];
    // cube.h -> make_cube_wireframe
    make_cube_wireframe(data, x, y, z, n);
    return sdl_gl_helper::gen_buffer(sizeof(data), data);
}

std::uint32_t world::gen_cube_buffer(const float x, float y, float z, float n, int w) const noexcept
{
    GLfloat* data = sdl_gl_helper::malloc_faces(10, 6);
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
    return sdl_gl_helper::gen_faces(10, 6, data);
}

std::uint32_t world::gen_plant_buffer(const float x, const float y, const float z, const float n,
                                      const int w) const noexcept
{
    GLfloat* data = sdl_gl_helper::malloc_faces(10, 4);
    float ao = 0;
    float light = 1;
    make_plant(data, ao, light, x, y, z, n, w, 45);
    return sdl_gl_helper::gen_faces(10, 4, data);
}

std::uint32_t world::gen_player_buffer(const float x, const float y, const float z, const float rx,
                                       const float ry) const noexcept
{
    GLfloat* data = sdl_gl_helper::malloc_faces(10, 6);
    make_player(data, x, y, z, rx, ry);
    return sdl_gl_helper::gen_faces(10, 6, data);
}

std::uint32_t world::gen_text_buffer(float x, const float y, const float n, const std::string_view text) const noexcept
{
    const auto length = static_cast<GLsizei>(text.size());
    GLfloat* data = sdl_gl_helper::malloc_faces(4, length);
    for (int i = 0; i < length; i++)
    {
        make_character(data + i * 24, x, y, n / 2, n, text[i]);
        x += n;
    }
    return sdl_gl_helper::gen_faces(4, length, data);
}

std::optional<scene_node*> world::find_chunk(const int p, const int q) const noexcept
{
    // Iterate through all active chunks in the BACKGROUND layer
    // NOTE: Start at index 1 because index 0 is the root layer node
    // Iterate up to m_next_chunk_slot (exclusive) which points to the next available slot
    const auto& background_layer = m_scene_layers[static_cast<std::size_t>(Layer::BACKGROUND)];
    for (std::size_t i = 1; i < m_next_chunk_slot && i < background_layer.size(); ++i)
    {
        scene_node* chunk = background_layer[i];
        if (chunk != nullptr && chunk->p == p && chunk->q == q)
        {
            return chunk;
        }
    }
    return std::nullopt;
}

int world::chunk_distance(const scene_node* chunk, const int p, const int q) noexcept
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

    compute_sight_vector(rx, ry, std::ref(vx), std::ref(vy), std::ref(vz));

    const auto& background_layer = m_scene_layers[static_cast<std::size_t>(Layer::BACKGROUND)];
    // NOTE: Start at index 1 because index 0 is the root layer node
    for (std::size_t i = 1; i < m_next_chunk_slot; ++i)
    {
        const scene_node* chunk = background_layer[i];
        if (chunk == nullptr || chunk_distance(chunk, p, q) > 1)
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
    const scene_node* chunk = chunk_opt.value();
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

void world::gen_sign_buffer(scene_node* chunk) const noexcept
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
    GLfloat* data = sdl_gl_helper::malloc_faces(5, max_faces);
    std::size_t faces = 0;
    for (int i = 0; i < signs->size; i++)
    {
        Sign* e = signs->data + i;
        faces += static_cast<std::size_t>(_gen_sign_buffer(data + static_cast<int>(faces) * 30,
                                                           static_cast<float>(e->x),
                                                           static_cast<float>(e->y),
                                                           static_cast<float>(e->z), e->face, e->text));
    }

    sdl_gl_helper::del_buffer(chunk->sign_buffer);
    chunk->sign_buffer = sdl_gl_helper::gen_faces(5, static_cast<GLsizei>(faces), data);
    chunk->sign_faces = static_cast<int>(faces);
}

int world::has_lights(scene_node* chunk) const noexcept
{
    for (int dp = -1; dp <= 1; dp++)
    {
        for (int dq = -1; dq <= 1; dq++)
        {
            scene_node* other = chunk;
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

void world::dirty_chunk(scene_node* chunk) const noexcept
{
    chunk->dirty = 1;
    if (has_lights(chunk))
    {
        // OPTIMIZATION: Only mark direct neighbors as dirty, not diagonals
        // This reduces the number of chunks that need updating from 9 to 5
        for (int dp = -1; dp <= 1; dp++)
        {
            for (int dq = -1; dq <= 1; dq++)
            {
                // Skip diagonal neighbors - they'll get updated if needed
                if (dp != 0 && dq != 0)
                {
                    continue;
                }

                auto other_opt = find_chunk(chunk->p + dp, chunk->q + dq);
                if (other_opt.has_value())
                {
                    other_opt.value()->dirty = 1;
                }
            }
        }
    }
}

void world::update_dirty_chunks_async() noexcept
{
    // OPTIMIZATION: Delegate dirty chunk updates to worker threads
    // This prevents blocking the main render thread when placing/destroying blocks

    // Process dirty chunks through the worker system
    for (auto&& worker : m_workers)
    {
        worker->mtx.lock();
        if (worker->state == WorkerState::IDLE)
        {
            // Find a dirty chunk that needs updating and is assigned to this worker
            // NOTE: Start at index 1 because index 0 is the root layer node
            const auto& background_layer = m_scene_layers[static_cast<std::size_t>(Layer::BACKGROUND)];
            for (std::size_t i = 1; i < m_next_chunk_slot && i < background_layer.size(); ++i)
            {
                scene_node* chunk = background_layer[i];
                if (chunk->dirty)
                {
                    int index = (SDL_abs(chunk->p) ^ SDL_abs(chunk->q)) % m_workers.size();
                    if (index == worker->index)
                    {
                        // Assign this dirty chunk to the worker
                        worker_item* item = &worker->item;
                        item->p = chunk->p;
                        item->q = chunk->q;
                        item->load = 0;

                        // Set up neighbor maps for chunk generation
                        for (int dp = -1; dp <= 1; dp++)
                        {
                            for (int dq = -1; dq <= 1; dq++)
                            {
                                scene_node* other = chunk;
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
                                    item->block_maps[dp + 1][dq + 1] = nullptr;
                                    item->light_maps[dp + 1][dq + 1] = nullptr;
                                }
                            }
                        }

                        worker->state = WorkerState::BUSY;
                        worker->cnd.notify_one();
                        break;  // Assigned one chunk to this worker, move to next worker
                    }
                }
            }
        }
        worker->mtx.unlock();
    }
}

void world::occlusion(char neighbors[27], char lights[27], float shades[27], float ao[6][4],
                      float light[6][4]) noexcept
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
            const float total = curve[value] + shade_sum / 4.0f;
            ao[i][j] = SDL_min(total, 1.0f);
            light[i][j] = light_sum / 15.0f / 4.0f;
        }
    }
} // occlusion

void world::light_fill(char* opaque, char* light, int x, int y, int z, int w, int force) noexcept
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
void world::compute_chunk(worker_item* item) const noexcept
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
            if (Map* map = item->light_maps[a][b]; map && map->size)
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
    GLfloat* data = sdl_gl_helper::malloc_faces(components, faces);
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

void world::generate_chunk(scene_node* chunk, worker_item* item) const noexcept
{
    chunk->miny = item->miny;
    chunk->maxy = item->maxy;
    chunk->faces = item->faces;
    sdl_gl_helper::del_buffer(chunk->buffer);
    chunk->buffer = sdl_gl_helper::gen_faces(10, item->faces, item->data);
    this->gen_sign_buffer(chunk);
}

void world::gen_chunk_buffer(scene_node* chunk) const noexcept
{
    worker_item _item;
    worker_item* item = &_item;
    item->p = chunk->p;
    item->q = chunk->q;
    for (int dp = -1; dp <= 1; dp++)
    {
        for (int dq = -1; dq <= 1; dq++)
        {
            scene_node* other = chunk;
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
void world::load_chunk(worker_item* item) const noexcept
{
    int p = item->p;
    int q = item->q;

    Map* block_map = item->block_maps[1][1];
    Map* light_map = item->light_maps[1][1];

    create_world(p, q, map_set_func, block_map, BUILD_CHUNK_SIZE);
    db_load_blocks(block_map, p, q);
    db_load_lights(light_map, p, q);
}

void world::init_chunk(scene_node* chunk, int p, int q) const noexcept
{
    chunk->p = p;
    chunk->q = q;
    chunk->faces = 0;
    chunk->sign_faces = 0;
    chunk->buffer = 0;
    chunk->sign_buffer = 0;
    chunk->set_category(Entity::CHUNK);
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

    // Attach to spatial hierarchy (cast away const since we're modifying scene graph)
    const_cast<world*>(this)->attach_chunk_to_layer(chunk, static_cast<int>(Layer::BACKGROUND));
}

void world::create_chunk(scene_node* chunk, int p, int q) const noexcept
{
    init_chunk(chunk, p, q);

    worker_item _item;
    worker_item* item = &_item;
    item->p = chunk->p;
    item->q = chunk->q;
    item->block_maps[1][1] = &chunk->map;
    item->light_maps[1][1] = &chunk->lights;

    load_chunk(item);
}

void world::delete_chunks() noexcept
{
    std::size_t count = this->m_next_chunk_slot;
    const player::state* s1 = &m_player->s1;
    auto& background_layer = m_scene_layers[static_cast<std::size_t>(Layer::BACKGROUND)];

    // NOTE: Start at index 1 because index 0 is the root layer node
    for (std::size_t i = 1; i < count; ++i)
    {
        scene_node* chunk = background_layer[i];
        if (chunk == nullptr) continue;

        int remove_chunk = 1;
        int p = chunked(s1->x);
        int q = chunked(s1->z);
        if (chunk_distance(chunk, p, q) < DELETE_CHUNK_RADIUS)
        {
            remove_chunk = 0;
            break;
        }
        if (remove_chunk)
        {
            // Detach from spatial hierarchy first
            detach_chunk_from_layer(chunk);

            map_free(&chunk->map);
            map_free(&chunk->lights);
            sign_list_free(&chunk->signs);
            sdl_gl_helper::del_buffer(chunk->buffer);
            sdl_gl_helper::del_buffer(chunk->sign_buffer);

            // Move the last chunk to this position
            --count;
            const scene_node* other = background_layer[count];

            // Before copying, detach the other chunk too
            if (other != nullptr)
            {
                detach_chunk_from_layer(const_cast<scene_node*>(other));
                SDL_memcpy(chunk, other, sizeof(scene_node));

                // Reattach after copy (the moved chunk needs to update its parent reference)
                if (chunk->get_category() == Entity::CHUNK)
                {
                    attach_chunk_to_layer(chunk, static_cast<int>(Layer::BACKGROUND));
                }

                // Null out the last position
                background_layer[count] = nullptr;
            }
        }
    }
    this->m_next_chunk_slot = count;
}

void world::delete_all_chunks() noexcept
{
    auto& background_layer = m_scene_layers[static_cast<std::size_t>(Layer::BACKGROUND)];

    // NOTE: Start at index 1 because index 0 is the root layer node
    for (std::size_t i = 1; i < this->m_next_chunk_slot; ++i)
    {
        scene_node* chunk = background_layer[i];
        if (chunk == nullptr) continue;

        // Detach from spatial hierarchy
        detach_chunk_from_layer(chunk);

        map_free(&chunk->map);
        map_free(&chunk->lights);
        sign_list_free(&chunk->signs);
        sdl_gl_helper::del_buffer(chunk->buffer);
        sdl_gl_helper::del_buffer(chunk->sign_buffer);

        // Delete the chunk node
        delete chunk;
        background_layer[i] = nullptr;
    }
    // Reset to 1 to preserve root layer node at index 0
    this->m_next_chunk_slot = 1;

    // Clear the root node children in the background layer
    if (background_layer[0] != nullptr && background_layer[0]->get_category() == Entity::SCENE)
    {
        background_layer[0]->children.clear();
    }
}

void world::check_workers() noexcept
{
    for (auto&& w : m_workers)
    {
        w->mtx.lock();
        if (w->state == WorkerState::DONE)
        {
            worker_item* item = &w->item;
            if (auto chunk_opt = find_chunk(item->p, item->q); chunk_opt.has_value())
            {
                scene_node* chunk = chunk_opt.value();
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
            w->state = WorkerState::IDLE;
        }
        w->mtx.unlock();
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
                scene_node* chunk = chunk_opt.value();
                // OPTIMIZATION: Only regenerate if chunk has no buffer at all
                // Otherwise let worker threads handle dirty chunks asynchronously
                if (chunk->dirty && chunk->buffer == 0)
                {
                    gen_chunk_buffer(chunk);
                }
                // Dirty chunks with existing buffers will be updated by workers
            }
            else if (this->m_next_chunk_slot < MAX_CHUNKS)
            {
                auto& background_layer = m_scene_layers[static_cast<std::size_t>(Layer::BACKGROUND)];
                scene_node* chunk = new scene_node{};
                background_layer[this->m_next_chunk_slot] = chunk;
                ++this->m_next_chunk_slot;  // Move to next available slot
                create_chunk(chunk, a, b);
                gen_chunk_buffer(chunk);
            }
        }
    }
}

void world::ensure_chunks_worker(player* _player, worker* w) noexcept
{
    auto [width, height] = m_sdl->get_window_size();
    player::state* s = &_player->s1;
    float matrix[16];
    set_matrix_3d(matrix, width, height,
                  s->x, s->y, s->z, s->rx, s->ry, m_model.fov, m_model.is_ortho,
                  RENDER_CHUNK_RADIUS);
    float planes[6][4];
    frustum_planes(planes, RENDER_CHUNK_RADIUS, matrix);
    int p = chunked(s->x);
    int q = chunked(s->z);

    int start = 0x0fffffff;
    int best_score = start;
    int best_a = 0;
    int best_b = 0;
    for (int dp = -CREATE_CHUNK_RADIUS; dp <= CREATE_CHUNK_RADIUS; dp++)
    {
        for (int dq = -CREATE_CHUNK_RADIUS; dq <= CREATE_CHUNK_RADIUS; dq++)
        {
            int a = p + dp;
            int b = q + dq;
            int index = (SDL_abs(a) ^ SDL_abs(b)) % NUM_WORKERS;
            if (index != w->index)
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
                scene_node* chunk = chunk_opt.value();
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
    scene_node* chunk = nullptr;
    // Check if the chunk is already loaded
    if (!chunk_opt.has_value())
    {
        load = 1;
        if (this->m_next_chunk_slot < MAX_CHUNKS)
        {
            auto& background_layer = m_scene_layers[static_cast<std::size_t>(Layer::BACKGROUND)];
            chunk = new scene_node{};
            background_layer[this->m_next_chunk_slot] = chunk;
            ++this->m_next_chunk_slot;  // Move to next available slot
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
    worker_item* item = &w->item;
    item->p = chunk->p;
    item->q = chunk->q;
    item->load = load;
    for (int dp = -1; dp <= 1; dp++)
    {
        for (int dq = -1; dq <= 1; dq++)
        {
            scene_node* other = chunk;
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
    w->state = WorkerState::BUSY;
    w->cnd.notify_one();
} // ensure chunks worker

void world::ensure_chunks(player* _player) noexcept
{
    check_workers();
    force_chunks(_player);
    for (auto&& w : m_workers)
    {
        w->mtx.lock();
        if (w->state == WorkerState::IDLE)
        {
            ensure_chunks_worker(_player, w.get());
        }
        w->mtx.unlock();
    }
}

void world::unset_sign(const int x, const int y, const int z) const noexcept
{
    const int p = chunked(static_cast<float>(x));
    const int q = chunked(static_cast<float>(z));
    if (const auto chunk_opt = find_chunk(p, q); chunk_opt.has_value())
    {
        scene_node* chunk = chunk_opt.value();
        if (auto* signs = &chunk->signs; sign_list_remove_all(signs, x, y, z))
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

void world::unset_sign_face(const int x, const int y, const int z, const int face) const noexcept
{
    const int p = chunked(static_cast<float>(x));
    const int q = chunked(static_cast<float>(z));
    if (const auto chunk_opt = find_chunk(p, q); chunk_opt.has_value())
    {
        scene_node* chunk = chunk_opt.value();
        if (auto* signs = &chunk->signs; sign_list_remove(signs, x, y, z, face))
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

void world::_set_sign(const int p, const int q, const int x, const int y, const int z,
    const int face, const std::string_view text, const int dirty) const noexcept
{
    if (text.empty())
    {
        unset_sign_face(x, y, z, face);
        return;
    }
    if (const auto chunk_opt = find_chunk(p, q); chunk_opt.has_value())
    {
        scene_node* chunk = chunk_opt.value();
        auto* signs = &chunk->signs;
        sign_list_add(signs, x, y, z, face, text.data());
        if (dirty)
        {
            chunk->dirty = 1;
        }
    }
    db_insert_sign(p, q, x, y, z, face, text.data());
}

void world::set_sign(const int x, const int y, const int z, const int face, const std::string_view text) const noexcept
{
    int p = chunked(static_cast<float>(x));
    int q = chunked(static_cast<float>(z));
    _set_sign(p, q, x, y, z, face, text, 1);
}

void world::toggle_light(int x, int y, int z) const noexcept
{
    const int p = chunked(static_cast<float>(x));
    const int q = chunked(static_cast<float>(z));
    if (const auto chunk_opt = find_chunk(p, q); chunk_opt.has_value())
    {
        scene_node* chunk = chunk_opt.value();
        Map* map = &chunk->lights;
        const int w = map_get(map, x, y, z) ? 0 : 15;
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
        scene_node* chunk = chunk_opt.value();
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
        scene_node* chunk = chunk_opt.value();
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
        scene_node* chunk = chunk_opt.value();
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

int world::render_chunks(const sdl_gl_helper::attrib* attrib, player* _player, const std::uint32_t texture) const noexcept
{
    auto [width, height] = m_sdl->get_window_size();
    int result = 0;
    const player::state* s = &_player->s1;
    const int p = chunked(s->x);
    const int q = chunked(s->z);
    const float light = get_daylight();
    float matrix[16];
    // matrix.cpp -> set_matrix_3d
    set_matrix_3d(
        matrix, width, height,
        s->x, s->y, s->z, s->rx, s->ry, m_model.fov, m_model.is_ortho,
        RENDER_CHUNK_RADIUS);

    float planes[6][4];
    // matrix.cpp -> frustum_planes
    frustum_planes(planes, RENDER_CHUNK_RADIUS, matrix);
    glUseProgram(attrib->program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform3f(attrib->camera, s->x, s->y, s->z);
    glUniform1i(attrib->sampler, 0);
    glUniform1f(attrib->extra2, light);
    glUniform1f(attrib->extra3, static_cast<GLfloat>(RENDER_CHUNK_RADIUS * BUILD_CHUNK_SIZE));
    glUniform1i(attrib->extra4, static_cast<int>(m_model.is_ortho));
    glUniform1f(attrib->timer, time_of_day());
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);

    int chunks_rendered = 0;
    int chunks_culled_distance = 0;
    int chunks_culled_frustum = 0;

    // Calculate bounds for spatial traversal (chunks within render radius)
    int min_p = p - RENDER_CHUNK_RADIUS;
    int min_q = q - RENDER_CHUNK_RADIUS;
    int max_p = p + RENDER_CHUNK_RADIUS;
    int max_q = q + RENDER_CHUNK_RADIUS;

    // Use spatial hierarchy traversal with bounds culling
    traverse_chunks_in_bounds(min_p, min_q, max_p, max_q, [&](scene_node* chunk)
    {
        // Additional distance check
        if (chunk_distance(chunk, p, q) > RENDER_CHUNK_RADIUS)
        {
            chunks_culled_distance++;
            return;
        }

        // Frustum culling
        if (!chunk_visible(planes, chunk->p, chunk->q, chunk->miny, chunk->maxy))
        {
            chunks_culled_frustum++;
            return;
        }

        // Render the chunk
        m_sdl->draw_chunk(attrib, chunk);
        result += chunk->faces;
        chunks_rendered++;
    });

    return result;
}

void world::render_signs(const sdl_gl_helper::attrib* attrib, const player* _player, const std::uint32_t sign) const noexcept
{
    auto [width, height] = m_sdl->get_window_size();
    const player::state* s = &_player->s1;
    const int p = chunked(s->x);
    const int q = chunked(s->z);
    float matrix[16];
    set_matrix_3d(
        matrix, width, height,
        s->x, s->y, s->z, s->rx, s->ry, m_model.fov, m_model.is_ortho,
        RENDER_CHUNK_RADIUS);
    float planes[6][4];
    frustum_planes(planes, RENDER_CHUNK_RADIUS, matrix);

    glUseProgram(attrib->program);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, sign);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform1i(attrib->sampler, 2);
    glUniform1i(attrib->extra1, 1);

    // Calculate bounds for spatial traversal (chunks within sign render radius)
    int min_p = p - m_model.sign_radius;
    int min_q = q - m_model.sign_radius;
    int max_p = p + m_model.sign_radius;
    int max_q = q + m_model.sign_radius;

    // Use spatial hierarchy traversal
    traverse_chunks_in_bounds(min_p, min_q, max_p, max_q, [&](scene_node* chunk)
    {
        if (chunk_distance(chunk, p, q) > m_model.sign_radius)
        {
            return;
        }
        if (!chunk_visible(planes, chunk->p, chunk->q, chunk->miny, chunk->maxy))
        {
            return;
        }
        m_sdl->draw_signs(attrib, chunk);
    });
}

void world::render_sign(const sdl_gl_helper::attrib* attrib, player* _player, const std::uint32_t sign) const noexcept
{
    int x, y, z, face;
    if (!hit_test_face(_player, &x, &y, &z, &face))
    {
        return;
    }

    auto [width, height] = m_sdl->get_window_size();
    const player::state* s = &_player->s1;
    float matrix[16];
    set_matrix_3d(
        matrix, width, height,
        s->x, s->y, s->z, s->rx, s->ry, m_model.fov, m_model.is_ortho,
        RENDER_CHUNK_RADIUS);
    glUseProgram(attrib->program);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, sign);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform1i(attrib->sampler, 2);
    glUniform1i(attrib->extra1, 1);
    char text[MAX_SIGN_LENGTH];
    SDL_strlcpy(text, "put maze here", MAX_SIGN_LENGTH);
    text[MAX_SIGN_LENGTH - 1] = '\0';
    GLfloat* data = sdl_gl_helper::malloc_faces(5, SDL_strlen(text));
    const int length = _gen_sign_buffer(data, static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), face,
                                  text);
    const GLuint buffer = sdl_gl_helper::gen_faces(5, length, data);
    m_sdl->draw_sign(attrib, buffer, length);
    sdl_gl_helper::del_buffer(buffer);
}

void world::render_players(const sdl_gl_helper::attrib* attrib, player* _player) const noexcept
{
    auto [width, height] = m_sdl->get_window_size();
    player::state* s = &_player->s1;
    float matrix[16];
    set_matrix_3d(
        matrix, width, height,
        s->x, s->y, s->z, s->rx, s->ry, m_model.fov, m_model.is_ortho,
        RENDER_CHUNK_RADIUS);
    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform3f(attrib->camera, s->x, s->y, s->z);
    glUniform1i(attrib->sampler, 0);
    glUniform1f(attrib->timer, time_of_day());

    m_sdl->draw_player(attrib, m_player);
}

void world::render_wireframe(const sdl_gl_helper::attrib* attrib, const player* _player) const noexcept
{
    auto [width, height] = m_sdl->get_window_size();
    const player::state* s = &_player->s1;
    float matrix[16];
    set_matrix_3d(
        matrix, width, height,
        s->x, s->y, s->z, s->rx, s->ry, m_model.fov, m_model.is_ortho,
        RENDER_CHUNK_RADIUS);
    int hx, hy, hz;
    if (const int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz); is_obstacle(hw))
    {
        glUseProgram(attrib->program);
        glLineWidth(1);
        glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
        const GLuint wireframe_buffer = gen_wireframe_buffer(static_cast<float>(hx), static_cast<float>(hy),
                                                       static_cast<float>(hz), 0.53f);
        m_sdl->draw_lines(attrib, wireframe_buffer, 3, 24);
        sdl_gl_helper::del_buffer(wireframe_buffer);
    }
}

void world::render_crosshairs(const sdl_gl_helper::attrib* attrib) const noexcept
{
    auto [width, height] = m_sdl->get_window_size();
    float matrix[16];
    set_matrix_2d(matrix, width, height);
    glUseProgram(attrib->program);
    glLineWidth(static_cast<GLfloat>(4 * m_model.scale));
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    const GLuint crosshair_buffer = gen_crosshair_buffer();
    m_sdl->draw_lines(attrib, crosshair_buffer, 2, 4);
    sdl_gl_helper::del_buffer(crosshair_buffer);
}

void world::render_item(const sdl_gl_helper::attrib* attrib, const std::uint32_t texture) const noexcept
{
    auto [width, height] = m_sdl->get_window_size();
    float matrix[16];
    set_matrix_item(matrix, width, height, m_sdl->get_scale_factor());
    glUseProgram(attrib->program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform3f(attrib->camera, 0, 0, 5);
    glUniform1i(attrib->sampler, 0);
    glUniform1f(attrib->timer, time_of_day());
    if (const int w = items[m_model.item_index]; is_plant(w))
    {
        const GLuint buffer = gen_plant_buffer(0, 0, 0, 0.5, w);
        m_sdl->draw_plant(attrib, buffer);
        sdl_gl_helper::del_buffer(buffer);
    }
    else
    {
        const GLuint buffer = gen_cube_buffer(0, 0, 0, 0.5, w);
        m_sdl->draw_cube(attrib, buffer);
        sdl_gl_helper::del_buffer(buffer);
    }
}

void world::render_text(const sdl_gl_helper::attrib* attrib, const std::uint32_t font,
                        const int justify, float x, const float y, const float n, const std::string_view text) const noexcept
{
    auto [w, h] = m_sdl->get_window_size();
    float matrix[16];
    set_matrix_2d(matrix, w, h);
    glUseProgram(attrib->program);
    glUniformMatrix4fv(attrib->matrix, 1, GL_FALSE, matrix);
    glUniform1i(attrib->sampler, 3);
    glUniform1i(attrib->extra1, 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, font);
    const GLsizei length = static_cast<GLsizei>(text.length());
    x -= n * justify * (length - 1) / 2;
    const GLuint buffer = gen_text_buffer(x, y, n, text);
    m_sdl->draw_text(attrib, buffer, length);
    sdl_gl_helper::del_buffer(buffer);
}

void world::on_light() const noexcept
{
    const player::state* s = &m_player->s1;
    int hx, hy, hz;
    if (const int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        hy > 0 && hy < 256 && is_destructable(hw))
    {
        toggle_light(hx, hy, hz);
    }
}

void world::on_left_click() noexcept
{
    const player::state* s = &m_player->s1;
    int hx, hy, hz;
    if (const auto hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        hy > 0 && hy < 256 && is_destructable(hw))
    {
        set_block(hx, hy, hz, 0);
        record_block(hx, hy, hz, 0);
#if defined(MAZE_DEBUG)
        SDL_Log("on_left_click(%d, %d, %d, %d, block_type: %d): ", hx, hy, hz, hw, items[m_model.item_index]);
#endif
        if (is_plant(get_block(hx, hy + 1, hz)))
        {
            set_block(hx, hy + 1, hz, 0);
        }
    }
}

void world::on_right_click() noexcept
{
    const player::state* s = &m_player->s1;
    int hx, hy, hz;
    if (const int hw = hit_test(1, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        hy > 0 && hy < 256 && is_obstacle(hw))
    {
        if (!player_intersects_block(2, s->x, s->y, s->z, hx, hy, hz))
        {
            set_block(hx, hy, hz, items[m_model.item_index]);
            record_block(hx, hy, hz, items[m_model.item_index]);
#if defined(MAZE_DEBUG)
            SDL_Log("on_right_click(%d, %d, %d, %d, block_type: %d): ", hx, hy, hz, hw,
                    items[m_model.item_index]);
#endif
        }
    }
}

void world::on_middle_click() noexcept
{
    const player::state* s = &m_player->s1;
    int hx, hy, hz;
    const int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
    for (int i = 0; i < item_count; i++)
    {
        if (items[i] == hw)
        {
            m_model.item_index = i;
#if defined(MAZE_DEBUG)
            SDL_Log("Copying item index: %d\n", i);
#endif
            break;
        }
    }
}

