#include "world.h"

#include <MazeBuilder/randomizer.h>

#include <noise/noise.h>

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
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <ranges>
#include <thread>

// World configs
#define DAY_LENGTH 600
#define MAX_TEXT_LENGTH 256
#define CREATE_CHUNK_RADIUS 10
#define RENDER_CHUNK_RADIUS 20
#define BUILD_CHUNK_SIZE 32
#define RENDER_SIGN_RADIUS 4
#define SHOW_CLOUDS true
#define DELETE_CHUNK_RADIUS 14
#define NUM_WORKERS 4

struct worker_item
{
    int p{};
    int q{};
    int load{};
    voxels_map* block_maps[3][3]{};
    voxels_map* light_maps[3][3]{};
    int miny{};
    int maxy{};
    int faces{};
    float* data{};
};

enum class WorkerState : int
{
    IDLE = 0,
    BUSY = 1,
    DONE = 2
};

namespace
{
    struct ivec3
    {
        int x;
        int y;
        int z;
    };

    ivec3 face_normal(const int face) noexcept
    {
        switch (face)
        {
        case 0: return {-1, 0, 0};
        case 1: return {1, 0, 0};
        case 2: return {0, 0, -1};
        case 3: return {0, 0, 1};
        case 8: return {0, -1, 0};
        default: return {0, 1, 0};
        }
    }

    ivec3 face_u_axis(const int face) noexcept
    {
        switch (face)
        {
        case 0: return {0, 0, 1};
        case 1: return {0, 0, -1};
        case 2: return {-1, 0, 0};
        case 3: return {1, 0, 0};
        case 4: return {1, 0, 0};
        case 5: return {0, 0, 1};
        case 6: return {-1, 0, 0};
        case 7: return {0, 0, -1};
        case 8: return {1, 0, 0};
        default: return {1, 0, 0};
        }
    }

    ivec3 face_v_axis(const int face) noexcept
    {
        switch (face)
        {
        case 0:
        case 1:
        case 2:
        case 3:
            return {0, -1, 0};
        case 4: return {0, 0, 1};
        case 5: return {-1, 0, 0};
        case 6: return {0, 0, -1};
        case 7: return {1, 0, 0};
        case 8: return {0, 0, 1};
        default: return {0, 0, 1};
        }
    }

}

struct worker
{
    int index{};
    WorkerState state{WorkerState::IDLE};
    std::thread thrd;
    std::mutex mtx;
    std::condition_variable cnd;
    worker_item item;
    bool should_stop{};
};

static sdl_gl_helper::attrib s_block_attrib, s_line_attrib, s_text_attrib, s_sky_attrib;

std::string gl_error_checker(const char* file, const int line) noexcept
{
    GLenum error_code;
    std::string error_str;
    while ((error_code = glGetError()) != GL_NO_ERROR)
    {
        switch (error_code)
        {
        case GL_INVALID_ENUM:
            {
                error_str += "INVALID_ENUM";
                break;
            }
        case GL_INVALID_VALUE:
            {
                error_str += "INVALID_VALUE";
                break;
            }
        case GL_INVALID_OPERATION:
            {
                error_str += "INVALID_OPERATION";
                break;
            }
        case GL_OUT_OF_MEMORY:
            {
                error_str += "OUT_OF_MEMORY";
                break;
            }
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            {
                error_str += "INVALID_FRAMEBUFFER_OPERATION";
                break;
            }
        default: break;
        }
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                     "OpenGL ERROR: %s\n\t\tFILE: %s, LINE: %d\n", error_str.c_str(), file, line);
    }
    return error_code == GL_NO_ERROR ? "" : error_str;
}

#define CHECK_GL_ERR() gl_error_checker(__FILE__, __LINE__)

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
      , m_next_chunk_slot{1}
      , m_command_queue{}
      , m_player{p}
      , m_sky_buffer{0}
      , m_projected_plane{}
{
    m_projected_plane.projected_texture = &m_textures.get(TextureIdentifier::MAZE);

    // Set bidirectional reference between player and world
    if (m_player)
    {
        m_player->set_world(this);
    }
}

world::~world()
{
    if (m_player)
    {
        m_player->set_world(nullptr);
    }
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

    std::fill(background_layer.begin() + 1, background_layer.end(), nullptr);

    // Initialize FOREGROUND layer similarly if needed in the future
    auto& foreground_layer = m_scene_layers[static_cast<std::size_t>(Layer::FOREGROUND)];
    std::ranges::fill(foreground_layer, nullptr);
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
    scene_node* root = m_scene_layers.at(static_cast<std::size_t>(Layer::BACKGROUND)).at(0);
    if (root == nullptr)
    {
        return;
    }

    // For now, use a simple spatial subdivision approach
    // Find or create an appropriate spatial node to hold this chunk

    if (scene_node* spatial_parent = root; spatial_parent != nullptr)
    {
        chunk->parent = spatial_parent;

        if (const bool is_attached = std::ranges::find(spatial_parent->children, chunk) != spatial_parent->children.
                cend();
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

    std::erase(chunk->parent->children, chunk);

    chunk->parent = nullptr;
}

void world::traverse_chunks(const std::function<void(scene_node*)>& callback) const noexcept
{
    // Traverse the spatial hierarchy
    scene_node* root = m_scene_layers.at(static_cast<std::size_t>(Layer::BACKGROUND)).at(0);
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
        if ((static_cast<int>(node->get_category())
            & static_cast<int>(Entity::CHUNK)) != 0)
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

void world::traverse_chunks_in_bounds(const int min_p, const int min_q, const int max_p, const int max_q,
                                      const std::function<void(scene_node*)>& callback) const noexcept
{
    // Convert chunk coordinates to world coordinates
    constexpr int CHUNK_SIZE = BUILD_CHUNK_SIZE;
    const int min_x = min_p * CHUNK_SIZE;
    const int min_z = min_q * CHUNK_SIZE;
    const int max_x = (max_p + 1) * CHUNK_SIZE - 1;
    const int max_z = (max_q + 1) * CHUNK_SIZE - 1;

    scene_node* root = m_scene_layers.at(static_cast<std::size_t>(Layer::BACKGROUND)).at(0);
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
        if ((static_cast<int>(node->get_category())
            & static_cast<int>(Entity::CHUNK)) != 0)
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
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    build_scene();

    init_worker_threads();

    // Force create initial chunks around player
    force_chunks(m_player);

    m_player->m_pos.y = static_cast<float>(highest_block(m_player->m_pos.x, m_player->m_pos.z) + 2);

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

    s_sky_attrib.program = m_shaders.get(ShaderIdentifier::SKY_SHADER).get();
    s_sky_attrib.position = 0;
    s_sky_attrib.normal = 1;
    s_sky_attrib.uv = 2;
    s_sky_attrib.matrix = glGetUniformLocation(s_sky_attrib.program, "matrix");
    s_sky_attrib.sampler = glGetUniformLocation(s_sky_attrib.program, "sampler");
    s_sky_attrib.timer = glGetUniformLocation(s_sky_attrib.program, "timer");

    m_sky_buffer = sdl_gl_helper::gen_sky_buffer();

    auto [vw, vh] = m_sdl->get_window_size_in_pixels();
    if (!m_bloom.init(
            vw, vh,
            m_shaders.get(ShaderIdentifier::BLOOM_BLUR_SHADER).get(),
            m_shaders.get(ShaderIdentifier::BLOOM_COMPOSITE_SHADER).get()))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "world::init - bloom_pass init failed\n");
    }
}

void world::update(const float delta_time, mazes::randomizer& rng) noexcept
{
    while (!m_command_queue.empty())
    {
        auto [action, _] = m_command_queue.front();
        m_command_queue.pop();
        action(*m_player, delta_time, std::ref(rng));
    }

    process_build_queue();

    // Apply gravity as continuous force (convert delta_time from ms to seconds)
    const float dt_seconds = delta_time / 1000.0f;

    // Only apply gravity when not flying
    if (!m_player->is_flying())
    {
        m_player->m_vel.vy += FORCE_DUE_TO_GRAVITY * dt_seconds;
    }
    else
    {
        // In flying mode, apply damping to vertical velocity to stop floating
        m_player->m_vel.vy *= 0.85f;
    }

    // Apply damping to horizontal velocity when not actively moving
    // This prevents velocity from persisting after keys are released
    constexpr float horizontal_damping = 0.80f;
    m_player->m_vel.vx *= horizontal_damping;
    m_player->m_vel.vz *= horizontal_damping;

    // Apply velocity to position
    m_player->m_pos.y += m_player->m_vel.vy * dt_seconds;

    int hx, hy, hz, face;
    if (hit_test_face(&hx, &hy, &hz, &face))
    {
        // Track target for building purposes
        m_projected_plane.target_x = hx;
        m_projected_plane.target_y = hy;
        m_projected_plane.target_z = hz;
        m_projected_plane.target_face = face;
        m_projected_plane.has_valid_target = true;
    }
    else
    {
        // No valid target (looking at sky/void) - can't build here, but preview stays visible
        m_projected_plane.has_valid_target = false;
    }

    // Apply collision detection (height = 2 blocks for player)
    // Skip collision when flying (allows clipping through blocks)
    if (!m_player->is_flying())
    {
        // Update ground state based on collision via helper (world is friend of player)
        if (const int collision_result = collide(2, &m_player->m_pos.x, &m_player->m_pos.y, &m_player->m_pos.z);
            collision_result == 1)
        {
            m_player->m_vel.vy = 0.0f;
            m_player->set_on_ground(true);
        }
        else
        {
            m_player->set_on_ground(false);
        }
    }
    else
    {
        // When flying, not on ground
        m_player->set_on_ground(false);
    }

    delete_chunks();
    sdl_gl_helper::del_buffer(m_player->get_buffer());
    ensure_chunks(m_player);
    update_dirty_chunks_async();
}

void world::draw() const noexcept
{
    CHECK_GL_ERR();

    // Set viewport to match window dimensions
    int viewport_width, viewport_height;
    SDL_GetWindowSizeInPixels(m_sdl->window, &viewport_width, &viewport_height);
    glViewport(0, 0, viewport_width, viewport_height);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);

    // --- Pass 1: scene geometry → MRT FBO ---
    m_bloom.bind_scene();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get texture IDs from texture manager
    const auto atlas_texture = m_textures.get(TextureIdentifier::ATLAS).get();
    const auto signs_texture = m_textures.get(TextureIdentifier::SIGNS).get();
    const auto sky_texture   = m_textures.get(TextureIdentifier::SKY).get();

    // Sky sphere: single output only (not a bloom-eligible emitter).
    m_bloom.set_single_mode();
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    render_sky(m_sky_buffer, sky_texture);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);

    // Geometry with possible HDR bright pixels → enable both MRT attachments.
    m_bloom.set_mrt_mode();
    const auto triangle_faces = render_chunks(atlas_texture);
    render_player(atlas_texture);

    // Item preview and maze preview are clean 2D UI overlays — no bloom contribution.
    m_bloom.set_single_mode();
    render_item(atlas_texture);
    render_plane();

    // UI / overlay elements — already in single mode, no redundant switch needed.
    render_signs(signs_texture);
    render_sign(signs_texture);

    std::array<char, 256> buffer{};
    SDL_snprintf(buffer.data(), buffer.size(), "[%d triangle faces, %zu chunks]",
                 triangle_faces, get_chunk_count());
    render_text(m_textures.get(TextureIdentifier::BITMAP_FONT).get(), 0,
                10, viewport_height - 15, 12.0f, buffer.data());
    render_text(m_textures.get(TextureIdentifier::BITMAP_FONT).get(), 0,
                10, viewport_height - 35, 12.0f, "Press E to preview");
    render_text(m_textures.get(TextureIdentifier::BITMAP_FONT).get(), 0,
                10, viewport_height - 55, 12.0f, "Press B to build");

    render_wireframe();
    render_crosshairs();
    
    // Render CAD features (Tier 1)
    render_grid_overlay();
    render_measurement_lines();
    render_hover_info();
    render_maze_preview_ghost();

    // --- Pass 2+3: Gaussian blur + composite bloom → default framebuffer ---
    const float bloom_str = m_player->m_configs.use_bloom_effect ? bloom_pass::BLOOM_STRENGTH : 0.0f;
    m_bloom.execute(bloom_str);
}

command_queue& world::get_command_queue() noexcept
{
    return m_command_queue;
}

void world::destroy_world()
{
    m_bloom.destroy();
    // Cleanup worker threads and chunks
    cleanup_worker_threads();
    delete_all_chunks();
    sdl_gl_helper::del_buffer(m_player->get_buffer());
}

void world::handle_event(const SDL_Event& event) noexcept
{
    if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
    {
        const int w = event.window.data1;
        const int h = event.window.data2;
        if (w > 0 && h > 0)
        {
            m_bloom.resize(w, h);
            glViewport(0, 0, w, h);
        }
    }
}

void world::create_world(const int p, const int q,
                         const world_func& func, voxels_map* m, const int chunk_size) noexcept
{
    constexpr int pad = 1;
    for (int dx = -pad; dx < chunk_size + pad; dx++)
    {
        for (int dz = -pad; dz < chunk_size + pad; dz++)
        {
            int flag = 1;
            if (dx < 0 || dz < 0 || dx >= chunk_size || dz >= chunk_size)
            {
                flag = -1;
            }
            const int x = p * chunk_size + dx;
            const int z = q * chunk_size + dz;

            // Build the environment
            const float f = simplex2(static_cast<float>(x) * 0.01f, static_cast<float>(z) * 0.01f,
                                     4, 0.5f, 2.f);
            const float g = simplex2(static_cast<float>(-x) * 0.01f, static_cast<float>(-z) * 0.01f,
                                     2, 0.9f, 2.f);
            const int mh = g * 32 + 16;
            auto h = static_cast<int>(f * static_cast<float>(mh));
            int w = 1;
            if (constexpr int t = 12; h <= t)
            {
                h = t;
                w = 2;
            }

            // sand and grass terrain
            for (int y = 0; y < h; y++)
            {
                func(x, y, z, w * flag, m);
            }

            if (w == 1)
            {
                // grass
                if (simplex2(static_cast<float>(-x) * 0.1f,
                             static_cast<float>(z) * 0.1f,
                             4,
                             0.8f, 2.0f) > 0.6f)
                {
                    func(x, h, z, 17 * flag, m);
                }
                // flowers
                if (simplex2(static_cast<float>(x) * 0.05f,
                             static_cast<float>(-z) * 0.05f,
                             4,
                             0.8f,
                             2.0f) > 0.7f)
                {
                    const auto w1 = 18.f + simplex2(static_cast<float>(x) * 0.1f,
                                                    static_cast<float>(z) * 0.1f,
                                                    4,
                                                    0.8f,
                                                    2.0f) * 7.f;

                    func(x, h, z, w1 * static_cast<float>(flag), m);
                }

                // trees
                int ok = 1;
                if (dx - 4 < 0 || dz - 4 < 0 ||
                    dx + 4 >= BUILD_CHUNK_SIZE || dz + 4 >= BUILD_CHUNK_SIZE)
                {
                    ok = 0;
                }
                if (ok && simplex2(static_cast<float>(x), static_cast<float>(z), 6, 0.5f, 2.0f) > 0.84f)
                {
                    for (int y = h + 3; y < h + 8; y++)
                    {
                        for (int ox = -3; ox <= 3; ox++)
                        {
                            for (int oz = -3; oz <= 3; oz++)
                            {
                                const int d = (ox * ox) + (oz * oz) +
                                    (y - (h + 4)) * (y - (h + 4));
                                if (d < 11)
                                {
                                    func(x + ox, y, z + oz, 15, m);
                                }
                            }
                        }
                    }
                    for (int y = h; y < h + 7; y++)
                    {
                        func(x, y, z, 5, m);
                    }
                }
            }
            // clouds
            if (SHOW_CLOUDS)
            {
                for (int y = 64; y < 72; y++)
                {
                    if (simplex3(
                        static_cast<float>(x) * 0.01f,
                        static_cast<float>(y) * 0.1f,
                        static_cast<float>(z) * 0.01f,
                        8,
                        0.5f,
                        2.0f) > 0.75f)
                    {
                        func(x, y, z, 16 * flag, m);
                    }
                }
            }
        }
    }
} // create_world

bool world::update_preview(const std::vector<std::uint8_t>& pixel_data,
                           const int width,
                           const int height) const noexcept
{
    if (pixel_data.empty() || width <= 0 || height <= 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid maze preview buffer\n");
        return false;
    }

    if (!m_projected_plane.projected_texture->update_from_memory(
        pixel_data.data(),
        width,
        height,
        static_cast<std::uint32_t>(TextureIdentifier::MAZE), false))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to update maze texture from memory\n");
        return false;
    }

    return true;
}

void world::finalize_buildings(const std::vector<std::uint8_t>& pixel_data,
                               int width, int height, int scale,
                               int wall_height, int item_type) noexcept
{
    // Store preview data in memory for reusable building
    m_current_preview.pixel_data = pixel_data;
    m_current_preview.width = width;
    m_current_preview.height = height;
    m_current_preview.scale = scale;
    m_current_preview.wall_height = wall_height;
    m_current_preview.item_type = item_type;
    m_current_preview.has_data = true;
    
    SDL_Log("Preview finalized: %dx%d, scale=%d, height=%d, item=%d\n", 
            width, height, scale, wall_height, item_type);
}

// Build the current preview at the targeted location
// Called when user presses 'B' (BUILD/PLACE_MAZE action)
// Uses stored preview data and builds at current crosshair target
void world::commit_preview_to_world(int item_type) noexcept
{
    // Check if we have preview data
    if (!m_current_preview.has_data)
    {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "No preview to build - press 'E' first to generate a preview\n");
        return;
    }

    const int face = m_projected_plane.target_face;
    const ivec3 normal = face_normal(face);
    const ivec3 axis_u = face_u_axis(face);
    const ivec3 axis_v = face_v_axis(face);

    // Anchor one cell off the hit block, along face normal.
    const int anchor_x = m_projected_plane.target_x + normal.x;
    const int anchor_y = m_projected_plane.target_y + normal.y;
    const int anchor_z = m_projected_plane.target_z + normal.z;

    // Calculate logical maze dimensions
    const int logical_width = m_current_preview.width / m_current_preview.scale;
    const int logical_height = m_current_preview.height / m_current_preview.scale;

    // Build the maze at the current target location
    int blocks_placed = 0;
    for (int cell_y = 0; cell_y < logical_height; ++cell_y)
    {
        for (int cell_x = 0; cell_x < logical_width; ++cell_x)
        {
            // Sample the center of each scaled cell region
            const int pix_x = cell_x * m_current_preview.scale + m_current_preview.scale / 2;
            const int pix_y = cell_y * m_current_preview.scale + m_current_preview.scale / 2;
            const int pixel_index = (pix_y * m_current_preview.width + pix_x) * 4;

            // Read RGB values
            const uint8_t r = m_current_preview.pixel_data[pixel_index + 0];
            const uint8_t g = m_current_preview.pixel_data[pixel_index + 1];
            const uint8_t b = m_current_preview.pixel_data[pixel_index + 2];

            // Check if pixel is black (wall)
            if (r < 50 && g < 50 && b < 50)
            {
                // Extrude wall thickness along the target face normal.
                for (int depth = 0; depth < m_current_preview.wall_height; ++depth)
                {
                    const int world_x = anchor_x + cell_x * axis_u.x + cell_y * axis_v.x + depth * normal.x;
                    const int world_y = anchor_y + cell_x * axis_u.y + cell_y * axis_v.y + depth * normal.y;
                    const int world_z = anchor_z + cell_x * axis_u.z + cell_y * axis_v.z + depth * normal.z;

                    set_block(world_x, world_y, world_z, item_type);
                    blocks_placed++;
                }
            }
        }
    }
}

void world::process_build_queue() noexcept
{
    for (auto& f : m_building_processes)
        f();
    m_building_processes.clear();
}

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
    return (static_cast<double>(SDL_GetTicks()) + static_cast<double>(m_player->m_configs.start_time) - static_cast<
        double>(m_player->m_configs.start_ticks)) / 1000.0;
}

float world::time_of_day() const noexcept
{
    if (m_player->m_configs.day_length <= 0)
    {
        return 0.5f;
    }
    auto t = static_cast<float>(get_time());
    t /= static_cast<float>(m_player->m_configs.day_length);
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

bool world::chunk_visible(float planes[6][4], const int p, const int q, const int miny, const int maxy) const noexcept
{
    const auto miny_f = static_cast<float>(miny);
    const auto maxy_f = static_cast<float>(maxy);
    const auto x = static_cast<float>(p * BUILD_CHUNK_SIZE - 1);
    const float z = static_cast<float>(q * BUILD_CHUNK_SIZE - 1);
    const float d = static_cast<float>(BUILD_CHUNK_SIZE + 1);
    const float points[8][3] = {
        {x + 0.f, miny_f, z + 0.f},
        {x + d, miny_f, z + 0.f},
        {x + 0.f, miny_f, z + d},
        {x + d, miny_f, z + d},
        {x + 0.f, maxy_f, z + 0.f},
        {x + d, maxy_f, z + 0.f},
        {x + 0.f, maxy_f, z + d},
        {x + d, maxy_f, z + d}
    };
    const int n = m_player->m_configs.ortho ? 4 : 6;
    for (int i = 0; i < n; i++)
    {
        int in = 0;
        int out = 0;
        for (int j = 0; j < 8; j++)
        {
            const float d1 =
                planes[i][0] * points[j][0] +
                planes[i][1] * points[j][1] +
                planes[i][2] * points[j][2] +
                planes[i][3];
            if (d1 < 0)
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
            return false;
        }
    }
    return true;
} // chunk_visible

int world::highest_block(const float x, const float z) const noexcept
{
    int result = -1;
    const int nx = static_cast<int>(SDL_roundf(x));
    const int nz = static_cast<int>(SDL_roundf(z));
    const int p = chunked(x);
    const int q = chunked(z);
    if (const auto chunk = find_chunk(p, q); chunk.has_value())
    {
        const voxels_map* map = &chunk.value()->map;
        for (const auto [ex, ey, ez, ew] : *map)
        {
            // item.h -> is_obstacle
            if (item::is_obstacle(ew) && ex == nx && ez == nz)
            {
                result = SDL_max(result, ey);
            }
        }
    }
    return result;
}

int world::_hit_test(const voxels_map* map, const float max_distance, const int previous,
                     float x, float y, float z, float vx, float vy, float vz, int* hx, int* hy, int* hz) noexcept
{
    static constexpr int m = 32;
    int px = 0;
    int py = 0;
    int pz = 0;
    for (int i = 0; i < max_distance * m; i++)
    {
        const int nx = SDL_lroundf(x);
        const int ny = SDL_lroundf(y);
        if (const int nz = SDL_lroundf(z); nx != px || ny != py || nz != pz)
        {
            if (const int hw = map->get(nx, ny, nz); hw > 0)
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
                    const float z, const float rx, const float ry,
                    int* bx, int* by, int* bz) const noexcept
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
            if (const auto d = SDL_sqrtf(SDL_powf(static_cast<float>(hx) - x, 2)
                    + SDL_powf(static_cast<float>(hy) - y, 2)
                    + SDL_powf(static_cast<float>(hz) - z, 2));
                best == 0 || d < best)
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

int world::hit_test_face(int* x, int* y, int* z, int* face) const noexcept
{
    const player::position* s = &m_player->m_pos;
    // item.h -> is_obstacle
    if (int w = this->hit_test(0, s->x, s->y, s->z, s->rx, s->ry, x, y, z);
        item::is_obstacle(w))
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
            const int top = static_cast<int>(((degrees + 45.f) / 90.f)) % 4;
            *face = 4 + top;
            return 1;
        }
        if (dx == 0 && dy == -1 && dz == 0)
        {
            // Bottom face uses a dedicated id distinct from top orientations.
            *face = 8;
            return 1;
        }
    }
    return 0;
}

int world::collide(const int height, float* x, float* y, float* z) const noexcept
{
    int result = 0;
    const int p = this->chunked(*x);
    const int q = this->chunked(*z);
    const auto chunk_opt = find_chunk(p, q);
    if (!chunk_opt.has_value())
    {
        return result;
    }
    const scene_node* chunk = chunk_opt.value();
    const voxels_map* map = &chunk->map;
    const int nx = static_cast<int>(SDL_roundf(*x));
    const int ny = static_cast<int>(SDL_roundf(*y));
    const int nz = static_cast<int>(SDL_roundf(*z));
    const float px = *x - nx;
    const float py = *y - ny;
    const float pz = *z - nz;
    for (int dy = 0; dy < height; dy++)
    {
        constexpr float pad = 0.25f;
        // item.h -> is_obstacle
        if (px < -pad && item::is_obstacle(map->get(nx - 1, ny - dy, nz)))
        {
            *x = nx - pad;
        }
        if (px > pad && item::is_obstacle(map->get(nx + 1, ny - dy, nz)))
        {
            *x = nx + pad;
        }
        if (py < -pad && item::is_obstacle(map->get(nx, ny - dy - 1, nz)))
        {
            *y = ny - pad;
            result = 1;
        }
        if (py > pad && item::is_obstacle(map->get(nx, ny - dy + 1, nz)))
        {
            *y = ny + pad;
            result = 1;
        }
        if (pz < -pad && item::is_obstacle(map->get(nx, ny - dy, nz - 1)))
        {
            *z = nz - pad;
        }
        if (pz > pad && item::is_obstacle(map->get(nx, ny - dy, nz + 1)))
        {
            *z = nz + pad;
        }
    }
    return result;
}

bool world::player_intersects_block(const int height, const float x, const float y, const float z,
                                    const int hx, const int hy, const int hz) noexcept
{
    const auto nx = static_cast<int>(SDL_roundf(x));
    const auto ny = static_cast<int>(SDL_roundf(y));
    const auto nz = static_cast<int>(SDL_roundf(z));
    for (int i = 0; i < height; i++)
    {
        if (nx == hx && ny - i == hy && nz == hz)
        {
            return true;
        }
    }
    return false;
}

bool world::has_lights(const scene_node* chunk) const noexcept
{
    static constexpr std::array<std::array<int, 2>, 9> offsets = {{
        {-1,-1},{-1,0},{-1,1},{0,-1},{0,0},{0,1},{1,-1},{1,0},{1,1}
    }};
    return std::ranges::any_of(offsets, [&](const auto& off)
    {
        const int dp = off[0], dq = off[1];
        const scene_node* other = (dp || dq)
            ? find_chunk(chunk->p + dp, chunk->q + dq).value_or(nullptr)
            : chunk;
        return other && other->lights.size() != 0;
    });
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

                if (auto other_opt = find_chunk(chunk->p + dp, chunk->q + dq);
                    other_opt.has_value())
                {
                    other_opt.value()->dirty = 1;
                }
            }
        }
    }
}

void world::update_dirty_chunks_async() const noexcept
{
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
                                    if (auto other_opt = find_chunk(chunk->p + dp, chunk->q + dq);
                                        !other_opt.has_value())
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
                        break; // Assigned one chunk to this worker, move to next worker
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
            const int corner = neighbors[lookup3[i][j][0]];
            const int side1 = neighbors[lookup3[i][j][1]];
            const int side2 = neighbors[lookup3[i][j][2]];
            const int value = side1 && side2 ? 3 : corner + side1 + side2;
            float shade_sum = 0;
            float light_sum = 0;
            const int is_light = lights[13] == 15;
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

void world::light_fill(char* opaque, char* light, const int x, const int y, const int z,
                       int w, const int force) noexcept
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
void world::compute_chunk(worker_item* item) noexcept
{
    auto* opaque = static_cast<char*>(SDL_calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char)));
    auto* light = static_cast<char*>(SDL_calloc(XZ_SIZE * XZ_SIZE * Y_SIZE, sizeof(char)));
    auto* highest = static_cast<char*>(SDL_calloc(XZ_SIZE * XZ_SIZE, sizeof(char)));

    int ox = item->p * BUILD_CHUNK_SIZE - BUILD_CHUNK_SIZE - 1;
    int oy = -1;
    int oz = item->q * BUILD_CHUNK_SIZE - BUILD_CHUNK_SIZE - 1;

    // check for lights
    int has_light = 0;
    for (int a = 0; a < 3; a++)
    {
        for (int b = 0; b < 3; b++)
        {
            if (voxels_map* map = item->light_maps[a][b]; map && map->size())
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
            voxels_map* block_map = item->block_maps[a][b];
            if (!block_map)
            {
                continue;
            }
            for (const auto [ex, ey, ez, ew] : *block_map)
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
                    opaque[XYZ(x, y, z)] = !item::is_transparent(w);
                    if (opaque[XYZ(x, y, z)])
                    {
                        highest[XZ(x, z)] = SDL_max(highest[XZ(x, z)], y);
                    }
            }
        }
    }

    // flood fill light intensities
    if (has_light)
    {
        for (int a = 0; a < 3; a++)
        {
            for (int b = 0; b < 3; b++)
            {
                voxels_map* map = item->light_maps[a][b];
                if (!map)
                {
                    continue;
                }
                for (const auto [ex, ey, ez, ew] : *map)
                    {
                        int x = ex - ox;
                        int y = ey - oy;
                        int z = ez - oz;
                        light_fill(opaque, light, x, y, z, ew, 1);
                    }
            }
        }
    }

    voxels_map* block_map = item->block_maps[1][1];

    // count exposed faces
    int miny = 256;
    int maxy = 0;
    int faces = 0;
    for (const auto [ex, ey, ez, ew] : *block_map)
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
            if (item::is_plant(ew))
            {
                total = 4;
            }
            miny = SDL_min(miny, ey);
            maxy = SDL_max(maxy, ey);
            faces += total;
        }

    // generate geometry
    // each vertex has 10 components (x, y, z, nx, ny, nz, u, v, ao, light)
    static constexpr int components = 10;
    GLfloat* data = sdl_gl_helper::malloc_faces(components, faces);
    int offset = 0;
    for (const auto [ex, ey, ez, ew] : *block_map)
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
                        if (int highest_index = XZ(x + dx, z + dz);
                            highest_index < XZ_SIZE * XZ_SIZE && y + dy <= highest[highest_index])
                        {
                            for (int oy1 = 0; oy1 < 8; oy1++)
                            {
                                if (opaque[XYZ(x + dx, y + dy + oy1, z + dz)])
                                {
                                    shades[index] = 1.0f - oy1 * 0.125f;
                                    break;
                                }
                            }
                        }
                        index++;
                    }
                }
            }
            float ao[6][4];
            float light2[6][4];
            occlusion(neighbors, lights, shades, ao, light2);
            if (item::is_plant(ew))
            {
                total = 4;
                float min_ao = 1;
                float max_light = 0;
                for (int a = 0; a < 6; a++)
                {
                    for (int b = 0; b < 4; b++)
                    {
                        min_ao = SDL_min(min_ao, ao[a][b]);
                        max_light = SDL_max(max_light, light2[a][b]);
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
                    data + offset, ao, light2,
                    f1, f2, f3, f4, f5, f6,
                    static_cast<float>(ex), static_cast<float>(ey), static_cast<float>(ez), 0.5f, ew);
            }
            offset += total * 60;
        }

    SDL_free(opaque);
    SDL_free(light);
    SDL_free(highest);

    item->miny = miny;
    item->maxy = maxy;
    item->faces = faces;
    item->data = data;
} // compute_chunk

void world::generate_chunk(scene_node* chunk, const worker_item* item) noexcept
{
    chunk->miny = item->miny;
    chunk->maxy = item->maxy;
    chunk->faces = item->faces;
    sdl_gl_helper::del_buffer(chunk->buffer);
    chunk->buffer = sdl_gl_helper::gen_faces(10, item->faces, item->data);
    sdl_gl_helper::gen_sign_buffer(chunk);
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
                if (auto other_opt = find_chunk(chunk->p + dp, chunk->q + dq);
                    !other_opt.has_value())
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

// Create a chunk that represents a unique portion of the world
// p, q represents the chunk key
void world::load_chunk(const worker_item* item) noexcept
{
    const int p = item->p;
    const int q = item->q;

    voxels_map* block_map = item->block_maps[1][1];
    voxels_map* light_map = item->light_maps[1][1];

    create_world(p, q, [](int x, int y, int z, int w, voxels_map* m){ m->set(x, y, z, w); }, block_map, BUILD_CHUNK_SIZE);
    db_load_blocks(block_map, p, q);
    db_load_lights(light_map, p, q);
}

void world::init_chunk(scene_node* chunk, int p, int q) noexcept
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
    voxels_map* block_map = &chunk->map;
    voxels_map* light_map = &chunk->lights;
    const int dx = p * BUILD_CHUNK_SIZE - 1;
    constexpr int dy = 0;
    const int dz = q * BUILD_CHUNK_SIZE - 1;
    block_map->init(dx, dy, dz, 0x7fff);
    light_map->init(dx, dy, dz, 0xf);

    this->attach_chunk_to_layer(chunk, static_cast<int>(Layer::BACKGROUND));
}

void world::create_chunk(scene_node* chunk, const int p, const int q) noexcept
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
    const player::position* s1 = &m_player->m_pos;
    auto& background_layer = m_scene_layers[static_cast<std::size_t>(Layer::BACKGROUND)];

    const int p = chunked(s1->x);
    const int q = chunked(s1->z);
    // NOTE: Start at index 1 because index 0 is the root layer node
    for (std::size_t i = 1; i < count; ++i)
    {
        scene_node* chunk = background_layer[i];
        if (chunk == nullptr) continue;

        if (chunk_distance(chunk, p, q) < DELETE_CHUNK_RADIUS)
            break;

        {
            // Detach from spatial hierarchy first
            detach_chunk_from_layer(chunk);

            sign_list_free(&chunk->signs);
            sdl_gl_helper::del_buffer(chunk->buffer);
            sdl_gl_helper::del_buffer(chunk->sign_buffer);

            // Move the last chunk to this position
            --count;

            // Before copying, detach the other chunk too
            if (const scene_node* other = background_layer[count]; other != nullptr)
            {
                detach_chunk_from_layer(const_cast<scene_node*>(other));

                *chunk = *other;

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
                    chunk->map    = *item->block_maps[1][1];
                    chunk->lights = *item->light_maps[1][1];
                }
                generate_chunk(chunk, item);
            }
            // Only delete the heap-allocated maps from the load (ensure_chunks_worker) path.
            // For dirty-chunk recomputes (load==0) the pointers are borrowed from live scene_nodes
            // and must NOT be freed.
            if (item->load)
            {
                for (int a = 0; a < 3; a++)
                {
                    for (int b = 0; b < 3; b++)
                    {
                        delete item->block_maps[a][b];
                        delete item->light_maps[a][b];
                        item->block_maps[a][b] = nullptr;
                        item->light_maps[a][b] = nullptr;
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
    player::position* s = &_player->m_pos;
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
                ++this->m_next_chunk_slot; // Move to next available slot
                create_chunk(chunk, a, b);
                gen_chunk_buffer(chunk);
            }
        }
    }
}

void world::ensure_chunks_worker(player* _player, worker* w) noexcept
{
    auto [width, height] = m_sdl->get_window_size();
    player::position* s = &_player->m_pos;
    float matrix[16];
    set_matrix_3d(matrix, width, height,
                  s->x, s->y, s->z, s->rx, s->ry,
                  m_player->m_configs.fov,
                  m_player->m_configs.ortho,
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
            const auto invisible = ~static_cast<int>(chunk_visible(planes, a, b, 0, item::TOTAL_BLOCKS));
            int priority = 0;
            if (chunk_opt.has_value())
            {
                scene_node* chunk = chunk_opt.value();
                priority = chunk->buffer & chunk->dirty;
            }
            // Check for chunk to update based on lowest score
            if (const int score = (invisible << 24) | (priority << 16) | distance; score < best_score)
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
            ++this->m_next_chunk_slot; // Move to next available slot
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
                item->block_maps[dp + 1][dq + 1] = new voxels_map{other->map};
                item->light_maps[dp + 1][dq + 1] = new voxels_map{other->lights};
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
    const int p = chunked(static_cast<float>(x));
    const int q = chunked(static_cast<float>(z));
    _set_sign(p, q, x, y, z, face, text, 1);
}

void world::toggle_light(int x, int y, int z) const noexcept
{
    const int p = chunked(static_cast<float>(x));
    const int q = chunked(static_cast<float>(z));
    if (const auto chunk_opt = find_chunk(p, q); chunk_opt.has_value())
    {
        scene_node* chunk = chunk_opt.value();
        voxels_map* map = &chunk->lights;
        const int w = map->get(x, y, z) ? 0 : 15;
        map->set(x, y, z, w);
        db_insert_light(p, q, x, y, z, w);
        dirty_chunk(chunk);
    }
}

void world::set_light(int p, int q, int x, int y, int z, int w) const noexcept
{
    if (auto chunk_opt = find_chunk(p, q); chunk_opt.has_value())
    {
        scene_node* chunk = chunk_opt.value();
        if (voxels_map* map = &chunk->lights; map->set(x, y, z, w))
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

void world::_set_block(const int p, int q, int x, int y, int z, const int w, const int dirty) const noexcept
{
    if (const auto chunk_opt = find_chunk(p, q); chunk_opt.has_value())
    {
        scene_node* chunk = chunk_opt.value();
        if (voxels_map* map = &chunk->map; map->set(x, y, z, w))
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

void world::set_block(const int x, const int y, const int z, const int w) const noexcept
{
    const int p = chunked(static_cast<float>(x));
    const int q = chunked(static_cast<float>(z));
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

int world::get_block(const int x, const int y, const int z) const noexcept
{
    const int p = chunked(static_cast<float>(x));
    const int q = chunked(static_cast<float>(z));
    if (auto chunk_opt = find_chunk(p, q); chunk_opt.has_value())
    {
        const scene_node* chunk = chunk_opt.value();
        const voxels_map* map = &chunk->map;
        return map->get(x, y, z);
    }
    return 0;
}

std::size_t world::get_chunk_count() const noexcept
{
    return this->m_next_chunk_slot - 1;
}

// ---------------------------------------------------------------------------
// Matrix setup helpers
// ---------------------------------------------------------------------------

std::pair<int,int> world::begin_3d_pass(const sdl_gl_helper::attrib* a, float matrix[16]) const noexcept
{
    auto [w, h] = m_sdl->get_window_size();
    const auto* s = &m_player->m_pos;
    set_matrix_3d(matrix, w, h,
        s->x, s->y, s->z, s->rx, s->ry,
        m_player->m_configs.fov, m_player->m_configs.ortho, RENDER_CHUNK_RADIUS);
    glUseProgram(a->program);
    glUniformMatrix4fv(a->matrix, 1, GL_FALSE, matrix);
    return {w, h};
}

std::pair<int,int> world::begin_sky_pass(const sdl_gl_helper::attrib* a, float matrix[16]) const noexcept
{
    auto [w, h] = m_sdl->get_window_size();
    const auto* s = &m_player->m_pos;
    // Sky sphere always renders from the origin with no ortho distortion.
    set_matrix_3d(matrix, w, h,
        0, 0, 0, s->rx, s->ry,
        m_player->m_configs.fov, 0, RENDER_CHUNK_RADIUS);
    glUseProgram(a->program);
    glUniformMatrix4fv(a->matrix, 1, GL_FALSE, matrix);
    return {w, h};
}

std::pair<int,int> world::begin_item_pass(const sdl_gl_helper::attrib* a, float matrix[16]) const noexcept
{
    auto [w, h] = m_sdl->get_window_size();
    set_matrix_item(matrix, w, h, m_sdl->get_scale_factor());
    glUseProgram(a->program);
    glUniformMatrix4fv(a->matrix, 1, GL_FALSE, matrix);
    return {w, h};
}

std::pair<int,int> world::begin_2d_pass(const sdl_gl_helper::attrib* a, float matrix[16]) const noexcept
{
    auto [w, h] = m_sdl->get_window_size();
    set_matrix_2d(matrix, w, h);
    glUseProgram(a->program);
    glUniformMatrix4fv(a->matrix, 1, GL_FALSE, matrix);
    return {w, h};
}

// ---------------------------------------------------------------------------
// Render routines
// ---------------------------------------------------------------------------

int world::render_chunks(const std::uint32_t texture) const noexcept
{
    float matrix[16];
    begin_3d_pass(&s_block_attrib, matrix);
    int result = 0;
    const player::position* s = &this->m_player->m_pos;
    const int p = chunked(s->x);
    const int q = chunked(s->z);
    const float light = get_daylight();
    float planes[6][4];
    frustum_planes(planes, RENDER_CHUNK_RADIUS, matrix);
    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureIdentifier::ATLAS));
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform3f(s_block_attrib.camera, s->x, s->y, s->z);
    glUniform1i(s_block_attrib.sampler, 0);
    glUniform1f(s_block_attrib.extra2, light);
    glUniform1f(s_block_attrib.extra3, static_cast<GLfloat>(RENDER_CHUNK_RADIUS * BUILD_CHUNK_SIZE));
    glUniform1i(s_block_attrib.extra4, static_cast<int>(m_player->m_configs.ortho));
    glUniform1f(s_block_attrib.timer, time_of_day());

    int chunks_rendered = 0;
    int chunks_culled_distance = 0;
    int chunks_culled_frustum = 0;

    // Calculate bounds for spatial traversal (chunks within render radius)
    int min_p = p - RENDER_CHUNK_RADIUS;
    int min_q = q - RENDER_CHUNK_RADIUS;
    int max_p = p + RENDER_CHUNK_RADIUS;
    int max_q = q + RENDER_CHUNK_RADIUS;

    // Use spatial hierarchy traversal with bounds culling
    traverse_chunks_in_bounds(min_p, min_q, max_p, max_q, [&](const scene_node* chunk)
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
        sdl_gl_helper::draw_chunk(&s_block_attrib, chunk);
        result += chunk->faces;
        chunks_rendered++;
    });

    return result;
}

void world::render_signs(const std::uint32_t sign) const noexcept
{
    float matrix[16];
    begin_3d_pass(&s_text_attrib, matrix);
    const player::position* s = &this->m_player->m_pos;
    const int p = chunked(s->x);
    const int q = chunked(s->z);
    float planes[6][4];
    frustum_planes(planes, RENDER_CHUNK_RADIUS, matrix);
    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureIdentifier::SIGNS));
    glBindTexture(GL_TEXTURE_2D, sign);
    glUniform1i(s_text_attrib.sampler, static_cast<unsigned int>(TextureIdentifier::SIGNS));
    glUniform1i(s_text_attrib.extra1, 1);

    // Calculate bounds for spatial traversal (chunks within sign render radius)
    const int min_p = p - RENDER_SIGN_RADIUS;
    const int min_q = q - RENDER_SIGN_RADIUS;
    const int max_p = p + RENDER_SIGN_RADIUS;
    const int max_q = q + RENDER_SIGN_RADIUS;

    // Use spatial hierarchy traversal
    traverse_chunks_in_bounds(min_p, min_q, max_p, max_q, [&](scene_node* chunk)
    {
        if (chunk_distance(chunk, p, q) > RENDER_SIGN_RADIUS)
        {
            return;
        }
        if (!chunk_visible(planes, chunk->p, chunk->q, chunk->miny, chunk->maxy))
        {
            return;
        }
        sdl_gl_helper::draw_signs(&s_text_attrib, chunk);
    });
}

void world::render_sign(const std::uint32_t sign) const noexcept
{
    int x, y, z, face;
    if (!hit_test_face(&x, &y, &z, &face))
    {
        return;
    }

    float matrix[16];
    begin_3d_pass(&s_text_attrib, matrix);
    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureIdentifier::SIGNS));
    glBindTexture(GL_TEXTURE_2D, sign);
    glUniform1i(s_text_attrib.sampler, static_cast<unsigned int>(TextureIdentifier::SIGNS));
    glUniform1i(s_text_attrib.extra1, 1);
    char text[MAX_SIGN_LENGTH];
    SDL_strlcpy(text, m_player->m_configs.tag.c_str(), MAX_SIGN_LENGTH);
    text[MAX_SIGN_LENGTH - 1] = '\0';
    GLfloat* data = sdl_gl_helper::malloc_faces(5, SDL_strlen(text));
    const int length = sdl_gl_helper::_gen_sign_buffer(data, static_cast<float>(x), static_cast<float>(y),
                                                       static_cast<float>(z), face,
                                                       text);
    const GLuint buffer = sdl_gl_helper::gen_faces(5, length, data);
    sdl_gl_helper::draw_sign(&s_text_attrib, buffer, length);
    sdl_gl_helper::del_buffer(buffer);
}

void world::render_sky(const std::uint32_t buffer, const std::uint32_t sky_tex) const noexcept
{
    float matrix[16];
    begin_sky_pass(&s_sky_attrib, matrix);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sky_tex);
    glUniform1i(s_sky_attrib.sampler, 0);
    glUniform1f(s_sky_attrib.timer, time_of_day());
    sdl_gl_helper::draw_triangles_3d(&s_sky_attrib, buffer, 512 * 3);
}

void world::render_wireframe() const noexcept
{
    float matrix[16];
    begin_3d_pass(&s_line_attrib, matrix);
    const player::position* s = &m_player->m_pos;
    int hx, hy, hz;
    if (const int hw = hit_test(0, s->x, s->y, s->z, s->rx, s->ry, &hx, &hy, &hz);
        item::is_obstacle(hw))
    {
        glLineWidth(1);
        const GLuint wireframe_buffer = sdl_gl_helper::gen_wireframe_buffer(
            static_cast<float>(hx), static_cast<float>(hy),
            static_cast<float>(hz), 0.53f);
        sdl_gl_helper::draw_lines(&s_line_attrib, wireframe_buffer, 3, 24);
        sdl_gl_helper::del_buffer(wireframe_buffer);
    }
}

void world::render_crosshairs() const noexcept
{
    float matrix[16];
    begin_2d_pass(&s_line_attrib, matrix);
    glLineWidth(static_cast<GLfloat>(4 * m_sdl->get_scale_factor()));
    const GLuint crosshair_buffer = m_sdl->gen_crosshair_buffer();
    sdl_gl_helper::draw_lines(&s_line_attrib, crosshair_buffer, 2, 4);
    sdl_gl_helper::del_buffer(crosshair_buffer);
}

// ============================================================================
// CAD Feature Rendering (Tier 1)
// ============================================================================

void world::render_hover_info() const noexcept
{
    if (!m_player->m_configs.show_hover_info)
        return;
        
    if (!m_projected_plane.has_valid_target)
        return;
    
    // Get viewport dimensions
    int viewport_width, viewport_height;
    SDL_GetWindowSizeInPixels(m_sdl->window, &viewport_width, &viewport_height);
    
    const int hx = m_projected_plane.target_x;
    const int hy = m_projected_plane.target_y;
    const int hz = m_projected_plane.target_z;
    const int face = m_projected_plane.target_face;
    const int block_type = get_block(hx, hy, hz);
    
    // Calculate distance from player
    const float dx = m_player->m_pos.x - static_cast<float>(hx);
    const float dy = m_player->m_pos.y - static_cast<float>(hy);
    const float dz = m_player->m_pos.z - static_cast<float>(hz);
    const float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    // Render info text near crosshair (slightly offset from center)
    const float x_offset = static_cast<float>(viewport_width) * 0.5f + 30.0f;
    const float y_base = static_cast<float>(viewport_height) * 0.5f;
    const float line_height = 15.0f;
    
    char info_buffer[256];
    const auto font_tex = m_textures.get(TextureIdentifier::BITMAP_FONT).get();
    
    // Block type
    SDL_snprintf(info_buffer, sizeof(info_buffer), "Block: %s", player::get_block_name(block_type));
    render_text(font_tex, 0, x_offset, y_base - line_height * 2, 10.0f, info_buffer);
    
    // Coordinates
    SDL_snprintf(info_buffer, sizeof(info_buffer), "Pos: (%d, %d, %d)", hx, hy, hz);
    render_text(font_tex, 0, x_offset, y_base - line_height, 10.0f, info_buffer);
    
    // Face
    SDL_snprintf(info_buffer, sizeof(info_buffer), "Face: %s", player::get_face_name(face));
    render_text(font_tex, 0, x_offset, y_base, 10.0f, info_buffer);
    
    // Distance
    SDL_snprintf(info_buffer, sizeof(info_buffer), "Dist: %.1f", distance);
    render_text(font_tex, 0, x_offset, y_base + line_height, 10.0f, info_buffer);
}

void world::render_grid_overlay() const noexcept
{
    if (!m_player->m_configs.show_grid)
        return;
    
    float matrix[16];
    begin_3d_pass(&s_line_attrib, matrix);
    
    const int grid_spacing = m_player->m_configs.grid_spacing;
    const int grid_size = 64; // Size in blocks from center
    
    // Grid at Y=0 (or player's Y level)
    const int grid_y = 0; // Could use: static_cast<int>(m_player->m_pos.y)
    
    // Set grid color with opacity
    const float opacity = m_player->m_configs.grid_opacity;
    glUniform4f(s_line_attrib.extra1, 0.5f, 0.5f, 0.5f, opacity);
    
    glLineWidth(1.0f);
    
    // Draw grid lines along X axis (parallel to Z)
    for (int x = -grid_size; x <= grid_size; x += grid_spacing)
    {
        const GLuint buffer = sdl_gl_helper::gen_line_buffer(
            static_cast<float>(x), static_cast<float>(grid_y), static_cast<float>(-grid_size),
            static_cast<float>(x), static_cast<float>(grid_y), static_cast<float>(grid_size)
        );
        sdl_gl_helper::draw_lines(&s_line_attrib, buffer, 3, 2);
        sdl_gl_helper::del_buffer(buffer);
    }
    
    // Draw grid lines along Z axis (parallel to X)
    for (int z = -grid_size; z <= grid_size; z += grid_spacing)
    {
        const GLuint buffer = sdl_gl_helper::gen_line_buffer(
            static_cast<float>(-grid_size), static_cast<float>(grid_y), static_cast<float>(z),
            static_cast<float>(grid_size), static_cast<float>(grid_y), static_cast<float>(z)
        );
        sdl_gl_helper::draw_lines(&s_line_attrib, buffer, 3, 2);
        sdl_gl_helper::del_buffer(buffer);
    }
    
    // Reset color
    glUniform4f(s_line_attrib.extra1, 1.0f, 1.0f, 1.0f, 1.0f);
}

void world::render_measurement_lines() const noexcept
{
    if (m_player->m_configs.active_cad_tool != player::CADTool::MEASURE_DISTANCE)
        return;
    
    // Only draw if we have at least one point
    if (!m_player->m_measure_point1.valid)
        return;
    
    float matrix[16];
    begin_3d_pass(&s_line_attrib, matrix);
    
    const auto& p1 = m_player->m_measure_point1;
    
    // Draw marker at first point
    glUniform4f(s_line_attrib.extra1, 1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    glLineWidth(3.0f);
    
    const float marker_size = 0.3f;
    GLuint marker_buffer = sdl_gl_helper::gen_line_buffer(
        static_cast<float>(p1.x) - marker_size, static_cast<float>(p1.y), static_cast<float>(p1.z),
        static_cast<float>(p1.x) + marker_size, static_cast<float>(p1.y), static_cast<float>(p1.z)
    );
    sdl_gl_helper::draw_lines(&s_line_attrib, marker_buffer, 3, 2);
    sdl_gl_helper::del_buffer(marker_buffer);
    
    // If we have second point, draw measurement line
    if (m_player->m_measure_point2.valid)
    {
        const auto& p2 = m_player->m_measure_point2;
        
        // Draw line between points
        glUniform4f(s_line_attrib.extra1, 0.0f, 1.0f, 1.0f, 1.0f); // Cyan
        const GLuint line_buffer = sdl_gl_helper::gen_line_buffer(
            static_cast<float>(p1.x), static_cast<float>(p1.y), static_cast<float>(p1.z),
            static_cast<float>(p2.x), static_cast<float>(p2.y), static_cast<float>(p2.z)
        );
        sdl_gl_helper::draw_lines(&s_line_attrib, line_buffer, 3, 2);
        sdl_gl_helper::del_buffer(line_buffer);
        
        // Draw marker at second point
        glUniform4f(s_line_attrib.extra1, 1.0f, 0.0f, 0.0f, 1.0f); // Red
        marker_buffer = sdl_gl_helper::gen_line_buffer(
            static_cast<float>(p2.x) - marker_size, static_cast<float>(p2.y), static_cast<float>(p2.z),
            static_cast<float>(p2.x) + marker_size, static_cast<float>(p2.y), static_cast<float>(p2.z)
        );
        sdl_gl_helper::draw_lines(&s_line_attrib, marker_buffer, 3, 2);
        sdl_gl_helper::del_buffer(marker_buffer);
        
        // Calculate and display distance
        const int dx = p2.x - p1.x;
        const int dy = p2.y - p1.y;
        const int dz = p2.z - p1.z;
        const float distance = std::sqrt(static_cast<float>(dx*dx + dy*dy + dz*dz));
        
        // Display distance text (2D overlay)
        int viewport_width, viewport_height;
        SDL_GetWindowSizeInPixels(m_sdl->window, &viewport_width, &viewport_height);
        
        char distance_text[128];
        SDL_snprintf(distance_text, sizeof(distance_text), 
                    "Distance: %.2f blocks (dx:%d dy:%d dz:%d)", 
                    distance, dx, dy, dz);
        
        render_text(m_textures.get(TextureIdentifier::BITMAP_FONT).get(), 
                   0, 10.0f, static_cast<float>(viewport_height) - 100.0f, 12.0f, distance_text);
    }
    
    // Reset color
    glUniform4f(s_line_attrib.extra1, 1.0f, 1.0f, 1.0f, 1.0f);
    glLineWidth(1.0f);
}

void world::render_maze_preview_ghost() const noexcept
{
    // Check if feature is enabled
    if (!m_player->m_configs.show_maze_preview_ghost)
    {
        static bool logged_disabled = false;
        if (!logged_disabled)
        {
            SDL_Log("Ghost preview: Feature disabled in config\n");
            logged_disabled = true;
        }
        return;
    }
    
    // Check if we have preview data
    if (!m_current_preview.has_data)
    {
        return;
    }
    
    // Check if we have a valid target to place the maze
    if (!m_projected_plane.has_valid_target)
    {
        static int no_target_count = 0;
        if (no_target_count++ % 60 == 0)  // Log every 60 frames (~1 second)
        {
            SDL_Log("Ghost preview: No valid target (point crosshair at a block surface)\n");
        }
        return;
    }
    
    static int log_counter = 0;
    const bool should_log = (log_counter++ % 60 == 0);  // Log every 60 frames (~1 second)
    
    if (should_log)
    {
        SDL_Log("Ghost preview at (%d,%d,%d) face=%d, preview: %dx%d, data_size=%zu\n", 
                m_projected_plane.target_x, m_projected_plane.target_y, m_projected_plane.target_z,
                m_projected_plane.target_face, m_current_preview.width, m_current_preview.height,
                m_current_preview.pixel_data.size());
    }
    
    // Calculate where the maze would be placed
    const int face = m_projected_plane.target_face;
    const ivec3 normal = face_normal(face);
    const ivec3 axis_u = face_u_axis(face);
    const ivec3 axis_v = face_v_axis(face);
    
    // Anchor one cell off the hit block, along face normal
    const int anchor_x = m_projected_plane.target_x + normal.x;
    const int anchor_y = m_projected_plane.target_y + normal.y;
    const int anchor_z = m_projected_plane.target_z + normal.z;
    
    // Calculate logical maze dimensions
    const int logical_width = m_current_preview.width / m_current_preview.scale;
    const int logical_height = m_current_preview.height / m_current_preview.scale;
    
    // Setup rendering for wireframe lines
    float matrix[16];
    begin_3d_pass(&s_line_attrib, matrix);
    
    // Line shader outputs cyan color by default (1.0 - red = cyan)
    // No need to set color uniform as shader hardcodes it
    glLineWidth(1.0f);
    
    int blocks_rendered = 0;
    int wall_pixels_found = 0;
    int blocks_skipped = 0;
    int blocks_culled = 0;
    
    // Collect all wireframe vertices into a single buffer for batching (huge performance gain)
    std::vector<float> wireframe_data;
    wireframe_data.reserve(100000);  // Pre-allocate for large mazes
    
    // Get player position for distance culling
    const float player_x = m_player->m_pos.x;
    const float player_y = m_player->m_pos.y;
    const float player_z = m_player->m_pos.z;
    constexpr float render_distance = 64.0f;  // Only render preview within this distance
    
    // Iterate through preview pixels and collect wireframe vertices
    for (int pix_row = 0; pix_row < m_current_preview.height; ++pix_row)
    {
        for (int pix_col = 0; pix_col < m_current_preview.width; ++pix_col)
        {
            const int idx = (pix_row * m_current_preview.width + pix_col) * 4;  // RGBA format
            if (idx + 3 >= static_cast<int>(m_current_preview.pixel_data.size()))
                continue;
            
            const auto r = m_current_preview.pixel_data[idx];
            const auto g = m_current_preview.pixel_data[idx + 1];
            const auto b = m_current_preview.pixel_data[idx + 2];
            
            // Wall = dark blue-gray (24, 28, 34) - this is what we want to render as wireframe
            if (r == 24 && g == 28 && b == 34)
            {
                wall_pixels_found++;
                
                // Convert pixel coordinates to logical maze coordinates
                const int logical_col = pix_col / m_current_preview.scale;
                const int logical_row = pix_row / m_current_preview.scale;
                
                // Render only the bottom and top levels for performance
                // (showing just the outline/footprint of the maze)
                for (int level = 0; level < m_current_preview.wall_height; level += std::max(1, m_current_preview.wall_height - 1))
                {
                    // Calculate world position
                    const int world_x = anchor_x + axis_u.x * logical_col + axis_v.x * logical_row;
                    const int world_y = anchor_y + axis_u.y * logical_col + axis_v.y * logical_row + level;
                    const int world_z = anchor_z + axis_u.z * logical_col + axis_v.z * logical_row;
                    
                    // Distance culling - skip blocks far from player
                    const float dx = static_cast<float>(world_x) - player_x;
                    const float dy = static_cast<float>(world_y) - player_y;
                    const float dz = static_cast<float>(world_z) - player_z;
                    const float dist_sq = dx*dx + dy*dy + dz*dz;
                    if (dist_sq > render_distance * render_distance)
                    {
                        blocks_culled++;
                        continue;
                    }
                    
                    // Skip if block already exists at this location
                    if (get_block(world_x, world_y, world_z) != 0)
                    {
                        blocks_skipped++;
                        continue;
                    }
                    
                    // Generate wireframe vertices for this cube (72 floats = 24 vertices)
                    float cube_data[72];
                    make_cube_wireframe(cube_data, 
                                       static_cast<float>(world_x),
                                       static_cast<float>(world_y),
                                       static_cast<float>(world_z),
                                       0.51f);
                    
                    // Append to batch buffer
                    wireframe_data.insert(wireframe_data.end(), cube_data, cube_data + 72);
                    blocks_rendered++;
                }
            }
        }
    }
    
    // Render all wireframes in a single draw call (massive performance improvement)
    if (!wireframe_data.empty())
    {
        const GLuint batch_buffer = sdl_gl_helper::gen_buffer(
            wireframe_data.size() * sizeof(float),
            wireframe_data.data()
        );
        
        sdl_gl_helper::draw_lines(&s_line_attrib, batch_buffer, 3, 
                                   static_cast<int>(wireframe_data.size() / 3));
        sdl_gl_helper::del_buffer(batch_buffer);
    }
    
    if (should_log)
    {
        SDL_Log("Ghost stats: wall_pixels=%d, rendered=%d, skipped=%d, culled=%d\n", 
                wall_pixels_found, blocks_rendered, blocks_skipped, blocks_culled);
    }
    
    // Reset line width
    glLineWidth(1.0f);
}

void world::render_item(const std::uint32_t texture) const noexcept
{
    const GLboolean depth_was_enabled = glIsEnabled(GL_DEPTH_TEST);

    // Render item preview as a UI-style overlay so it looks identical across camera modes.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    float matrix[16];
    begin_item_pass(&s_block_attrib, matrix);

    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureIdentifier::ATLAS));
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform3f(s_block_attrib.camera, 0, 0, 5);
    glUniform1i(s_block_attrib.sampler, 0);
    glUniform1f(s_block_attrib.timer, time_of_day());
    glUniform1i(s_block_attrib.extra4, 0);

    if (const int w = m_player->get_item(); item::is_plant(w))
    {
        const GLuint buffer = sdl_gl_helper::gen_plant_buffer(0, 0, 0, 0.5f, w);
        sdl_gl_helper::draw_plant(&s_block_attrib, buffer);
        sdl_gl_helper::del_buffer(buffer);
    }
    else
    {
        const GLuint buffer = sdl_gl_helper::gen_cube_buffer(0, 0, 0, 0.5f, w);
        sdl_gl_helper::draw_cube(&s_block_attrib, buffer);
        sdl_gl_helper::del_buffer(buffer);
    }

    glDepthMask(GL_TRUE);
    if (depth_was_enabled)
    {
        glEnable(GL_DEPTH_TEST);
    }
}

void world::render_player(const std::uint32_t texture) const noexcept
{
    // Only render player model in 3rd person mode (any non-zero ortho)
    if (m_player->m_configs.ortho < 1)
    {
        return;
    }

    float matrix[16];
    begin_3d_pass(&s_block_attrib, matrix);
    const player::position* s = &m_player->m_pos;
    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureIdentifier::ATLAS));
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform3f(s_block_attrib.camera, s->x, s->y, s->z);
    glUniform1i(s_block_attrib.sampler, 0);
    glUniform1f(s_block_attrib.timer, time_of_day());

    const bool is_isometric_mode =
        m_player->m_configs.ortho_view_mode == player::OrthoViewMode::ISOMETRIC;
    const bool is_orthographic_active = m_player->m_configs.ortho > 0;
    // Fallback: some UI paths can leave mode enum in Perspective while ortho camera is active.
    const bool use_isometric_fx = is_isometric_mode || is_orthographic_active;

    static bool logged_iso_player_fx = false;
    if (use_isometric_fx && !logged_iso_player_fx)
    {
        SDL_Log("Player marker FX enabled (mode=%d, ortho=%d)\n",
                static_cast<int>(m_player->m_configs.ortho_view_mode),
                m_player->m_configs.ortho);
        logged_iso_player_fx = true;
    }

    const float t = static_cast<float>(SDL_GetTicks()) / 1000.0f;

    // In isometric mode, keep marker at screen center by placing it along sight vector.
    // In other ortho modes, keep the legacy 3rd-person offset behavior.
    float px = s->x;
    float py = s->y - 0.5f;
    float pz = s->z;
    if (use_isometric_fx)
    {
        float vx, vy, vz;
        compute_sight_vector(s->rx, s->ry, vx, vy, vz);
        constexpr float marker_distance = 3.0f;
        px = s->x + vx * marker_distance;
        py = s->y + vy * marker_distance - 0.35f + 0.16f * SDL_sinf(t * 3.0f);
        pz = s->z + vz * marker_distance;
    }
    else
    {
        constexpr float offset_distance = 3.0f;
        px = s->x - offset_distance * SDL_sinf(s->rx);
        pz = s->z + offset_distance * SDL_cosf(s->rx);
    }

    const float animated_rx = use_isometric_fx
        ? (s->rx + 0.20f * SDL_sinf(t * 2.4f) + 0.10f * SDL_sinf(t * 6.1f))
        : s->rx;
    const float animated_ry = use_isometric_fx
        ? (s->ry + 0.08f * SDL_sinf(t * 1.9f + 0.7f))
        : s->ry;

    const GLboolean depth_was_enabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean cull_was_enabled = glIsEnabled(GL_CULL_FACE);
    if (use_isometric_fx)
    {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
    }

    const GLuint player_buffer = sdl_gl_helper::gen_player_buffer(px, py, pz, animated_rx, animated_ry);
    sdl_gl_helper::draw_player(&s_block_attrib, player_buffer);
    sdl_gl_helper::del_buffer(player_buffer);

    if (use_isometric_fx)
    {
        // Pulsing halo and particles around the centered player marker.
        float line_matrix[16];
        begin_3d_pass(&s_line_attrib, line_matrix);

        const float pulse_n = 0.55f + 0.10f * SDL_sinf(t * 3.8f);
        glLineWidth(2.0f);
        const GLuint halo = sdl_gl_helper::gen_wireframe_buffer(px, py, pz, pulse_n);
        sdl_gl_helper::draw_lines(&s_line_attrib, halo, 3, 24);
        sdl_gl_helper::del_buffer(halo);
        glLineWidth(1.0f);

        constexpr int particle_count = 10;
        for (int i = 0; i < particle_count; ++i)
        {
            const float phase = t * 2.0f + static_cast<float>(i) * 0.65f;
            const float radius = 0.90f + 0.16f * SDL_sinf(t * 2.2f + static_cast<float>(i));
            const float ox = SDL_cosf(phase) * radius;
            const float oy = 0.35f * SDL_sinf(phase * 1.37f);
            const float oz = SDL_sinf(phase) * radius;
            const float s_marker = 0.05f;

            const GLuint lx = sdl_gl_helper::gen_line_buffer(
                px + ox - s_marker, py + oy, pz + oz,
                px + ox + s_marker, py + oy, pz + oz);
            sdl_gl_helper::draw_lines(&s_line_attrib, lx, 3, 2);
            sdl_gl_helper::del_buffer(lx);

            const GLuint ly = sdl_gl_helper::gen_line_buffer(
                px + ox, py + oy - s_marker, pz + oz,
                px + ox, py + oy + s_marker, pz + oz);
            sdl_gl_helper::draw_lines(&s_line_attrib, ly, 3, 2);
            sdl_gl_helper::del_buffer(ly);
        }

        // Guaranteed on-screen marker in isometric view: center pulsing square + orbit particles.
        float overlay_matrix[16];
        auto [overlay_w, overlay_h] = begin_2d_pass(&s_line_attrib, overlay_matrix);
        const float cx = static_cast<float>(overlay_w) * 0.5f;
        const float cy = static_cast<float>(overlay_h) * 0.5f;
        const float r = 18.0f + 5.0f * SDL_sinf(t * 4.2f);

        const GLuint top = sdl_gl_helper::gen_line_buffer(cx - r, cy + r, 0.0f, cx + r, cy + r, 0.0f);
        sdl_gl_helper::draw_lines(&s_line_attrib, top, 3, 2);
        sdl_gl_helper::del_buffer(top);

        const GLuint right = sdl_gl_helper::gen_line_buffer(cx + r, cy + r, 0.0f, cx + r, cy - r, 0.0f);
        sdl_gl_helper::draw_lines(&s_line_attrib, right, 3, 2);
        sdl_gl_helper::del_buffer(right);

        const GLuint bottom = sdl_gl_helper::gen_line_buffer(cx + r, cy - r, 0.0f, cx - r, cy - r, 0.0f);
        sdl_gl_helper::draw_lines(&s_line_attrib, bottom, 3, 2);
        sdl_gl_helper::del_buffer(bottom);

        const GLuint left = sdl_gl_helper::gen_line_buffer(cx - r, cy - r, 0.0f, cx - r, cy + r, 0.0f);
        sdl_gl_helper::draw_lines(&s_line_attrib, left, 3, 2);
        sdl_gl_helper::del_buffer(left);

        constexpr int overlay_particles = 8;
        for (int i = 0; i < overlay_particles; ++i)
        {
            const float phase = t * 2.8f + static_cast<float>(i) * 0.785f;
            const float pr = r + 10.0f + 3.0f * SDL_sinf(t * 3.1f + static_cast<float>(i));
            const float px2 = cx + SDL_cosf(phase) * pr;
            const float py2 = cy + SDL_sinf(phase) * pr;
            const float s2 = 2.0f;

            const GLuint hp = sdl_gl_helper::gen_line_buffer(px2 - s2, py2, 0.0f, px2 + s2, py2, 0.0f);
            sdl_gl_helper::draw_lines(&s_line_attrib, hp, 3, 2);
            sdl_gl_helper::del_buffer(hp);

            const GLuint vp = sdl_gl_helper::gen_line_buffer(px2, py2 - s2, 0.0f, px2, py2 + s2, 0.0f);
            sdl_gl_helper::draw_lines(&s_line_attrib, vp, 3, 2);
            sdl_gl_helper::del_buffer(vp);
        }

        if (depth_was_enabled)
        {
            glEnable(GL_DEPTH_TEST);
        }
        if (cull_was_enabled)
        {
            glEnable(GL_CULL_FACE);
        }
    }
}

void world::render_plane() const noexcept
{
    if (!m_player || !m_player->m_configs.preview_enabled)
    {
        return;
    }

    // Check if we have a valid texture to display
    // Note: We don't check visibility or target since this is now a persistent 2D overlay
    // The target is still tracked for building purposes, but not required for display
    if (!m_projected_plane.projected_texture)
    {
        return;
    }

    // Only render if a maze texture has been generated (width/height > 0)
    const float tex_width = static_cast<float>(m_projected_plane.projected_texture->get_width());
    const float tex_height = static_cast<float>(m_projected_plane.projected_texture->get_height());

    if (tex_width <= 0.0f || tex_height <= 0.0f)
    {
        return;
    }

    float matrix[16];
    auto [width, height] = begin_2d_pass(&s_block_attrib, matrix);
    glUniform1i(s_block_attrib.sampler, static_cast<unsigned int>(TextureIdentifier::MAZE));
    glUniform1i(s_block_attrib.extra1, 0);

    // Bind the maze texture
    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureIdentifier::MAZE));
    glBindTexture(GL_TEXTURE_2D, m_textures.get(TextureIdentifier::MAZE).get());

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth testing for 2D overlay
    glDisable(GL_DEPTH_TEST);


    // Calculate preview size (5-10% of screen dimensions)
    constexpr float PREVIEW_SIZE_FACTOR = 0.075f; // 7.5% of screen dimensions
    const float preview_max_width = static_cast<float>(width) * PREVIEW_SIZE_FACTOR * 2.0f;
    const float preview_max_height = static_cast<float>(height) * PREVIEW_SIZE_FACTOR * 2.0f;

    // Calculate aspect ratio and fit preview within bounds
    const float tex_aspect = tex_width / tex_height;
    float preview_width, preview_height;

    if (tex_aspect > 1.0f) {
        // Wider than tall
        preview_width = preview_max_width;
        preview_height = preview_max_width / tex_aspect;
        if (preview_height > preview_max_height) {
            preview_height = preview_max_height;
            preview_width = preview_max_height * tex_aspect;
        }
    } else {
        // Taller than wide
        preview_height = preview_max_height;
        preview_width = preview_max_height * tex_aspect;
        if (preview_width > preview_max_width) {
            preview_width = preview_max_width;
            preview_height = preview_max_width / tex_aspect;
        }
    }

    // Position in bottom right corner with some padding
    constexpr float PADDING = 10.0f;
    const float bottom_right_x = static_cast<float>(width) - preview_width - PADDING;
    const float bottom_right_y = PADDING; // In 2D coords, 0 is at bottom

    // Vertex format: x, y, u, v (4 floats per vertex, 6 vertices for 2 triangles)
    const float x0 = bottom_right_x;
    const float y0 = bottom_right_y;
    const float x1 = bottom_right_x + preview_width;
    const float y1 = bottom_right_y + preview_height;

    const std::array<float, 6 * 4> quad_data = {
        x0, y0, 0.0f, 0.0f,  // tri1: bottom-left
        x1, y0, 1.0f, 0.0f,  // tri1: bottom-right
        x1, y1, 1.0f, 1.0f,  // tri1: top-right
        x0, y0, 0.0f, 0.0f,  // tri2: bottom-left
        x1, y1, 1.0f, 1.0f,  // tri2: top-right
        x0, y1, 0.0f, 1.0f,  // tri2: top-left
    };

    // Create and render the quad using 2D drawing
    const GLuint temp_buffer = sdl_gl_helper::gen_buffer(sizeof(quad_data), quad_data.data());
    sdl_gl_helper::draw_triangles_2d(&s_block_attrib, temp_buffer, 6);
    sdl_gl_helper::del_buffer(temp_buffer);

    // Restore GL state
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void world::render_text(const std::uint32_t font, const int justify,
                        float x, const float y, const float n, const std::string_view text) const noexcept
{
    while (glGetError() != GL_NO_ERROR)
    {
        // Clear prior errors so diagnostics below pinpoint the failing call.
    }

    const auto log_gl_error = [](const char* step)
    {
        if (const GLenum error = glGetError(); error != GL_NO_ERROR)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                         "OpenGL error in render_text (%s): 0x%x\n",
                         step, error);
        }
    };

    float matrix[16];
    begin_2d_pass(&s_text_attrib, matrix);
    log_gl_error("begin_2d_pass");

    glUniform1i(s_text_attrib.sampler, static_cast<int>(TextureIdentifier::BITMAP_FONT));
    log_gl_error("glUniform1i sampler");

    glUniform1i(s_text_attrib.extra1, 0);
    log_gl_error("glUniform1i extra1");

    glActiveTexture(GL_TEXTURE0 + static_cast<unsigned int>(TextureIdentifier::BITMAP_FONT));
    log_gl_error("glActiveTexture");

    glBindTexture(GL_TEXTURE_2D, font);
    log_gl_error("glBindTexture");

    const GLsizei length = static_cast<GLsizei>(text.length());
    if (length <= 0)
    {
        return;
    }

    x -= n * justify * (length - 1) / 2;
    const GLuint buffer = sdl_gl_helper::gen_text_buffer(x, y, n, text);
    log_gl_error("gen_text_buffer");

    if (buffer == 0)
    {
        return;
    }

    sdl_gl_helper::draw_text(&s_text_attrib, buffer, length);
    log_gl_error("draw_text");

    sdl_gl_helper::del_buffer(buffer);
    log_gl_error("del_buffer");
}
