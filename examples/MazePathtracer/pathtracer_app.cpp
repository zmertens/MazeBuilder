// MazePathtracer - 3D maze visualization using software path tracing + SDL3 GPU API
//
// Workflow:
//   1. Generate a 2D maze using MazeBuilder (DFS or Binary-Tree algorithm)
//   2. Translate the ASCII maze into a 3D scene of spheres (walls = spheres)
//   3. Software-ray-trace the scene on the CPU into a pixel buffer
//   4. Upload the pixel buffer to the GPU via SDL_GPU (transfer buffer → texture)
//   5. Blit the GPU texture to the swap-chain and present

#include "pathtracer_app.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

#include <MazeBuilder/configurator.h>
#include <MazeBuilder/create.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten_local/emscripten_mainloop_stub.h>
#endif

// ---------------------------------------------------------------------------
// Minimal 3-component vector math
// ---------------------------------------------------------------------------
struct vec3
{
    float x{0.f}, y{0.f}, z{0.f};

    [[nodiscard]] vec3 operator+(const vec3& o) const noexcept { return {x + o.x, y + o.y, z + o.z}; }
    [[nodiscard]] vec3 operator-(const vec3& o) const noexcept { return {x - o.x, y - o.y, z - o.z}; }
    [[nodiscard]] vec3 operator*(float t)       const noexcept { return {x * t,   y * t,   z * t  }; }
    [[nodiscard]] vec3 operator*(const vec3& o) const noexcept { return {x * o.x, y * o.y, z * o.z}; }
    [[nodiscard]] vec3 operator/(float t)       const noexcept { return {x / t,   y / t,   z / t  }; }
    vec3& operator+=(const vec3& o) noexcept { x += o.x; y += o.y; z += o.z; return *this; }

    [[nodiscard]] float length_sq() const noexcept { return x*x + y*y + z*z; }
    [[nodiscard]] float length()    const noexcept { return std::sqrt(length_sq()); }
};

[[nodiscard]] static inline float dot(const vec3& a, const vec3& b) noexcept
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

[[nodiscard]] static inline vec3 cross(const vec3& a, const vec3& b) noexcept
{
    return { a.y*b.z - a.z*b.y,
             a.z*b.x - a.x*b.z,
             a.x*b.y - a.y*b.x };
}

[[nodiscard]] static inline vec3 normalize(const vec3& v) noexcept
{
    const float len = v.length();
    if (len < 1e-8f) return {0.f, 1.f, 0.f};
    return v / len;
}

[[nodiscard]] static inline vec3 reflect(const vec3& v, const vec3& n) noexcept
{
    return v - n * (2.f * dot(v, n));
}

// ---------------------------------------------------------------------------
// Ray
// ---------------------------------------------------------------------------
struct ray
{
    vec3 origin;
    vec3 dir; // normalized

    [[nodiscard]] vec3 at(float t) const noexcept { return origin + dir * t; }
};

// ---------------------------------------------------------------------------
// Sphere (position, size, material)
// ---------------------------------------------------------------------------
struct sphere
{
    vec3  center;
    float radius;
    vec3  albedo;    // diffuse color [0, 1] per channel
    float roughness; // 0 = mirror-like, 1 = fully diffuse
};

/// @brief Ray-sphere intersection test.
/// @return true if the ray intersects [t_min, t_max]; sets t_hit to closest root.
[[nodiscard]] static bool hit_sphere(const sphere& s, const ray& r,
                                      float t_min, float t_max,
                                      float& t_hit) noexcept
{
    const vec3  oc  = r.origin - s.center;
    const float a   = dot(r.dir, r.dir);
    const float hb  = dot(oc, r.dir); // half-b coefficient
    const float c   = dot(oc, oc) - s.radius * s.radius;
    const float disc = hb * hb - a * c;

    if (disc < 0.f) return false;

    const float sq = std::sqrt(disc);

    // Near root
    float t = (-hb - sq) / a;
    if (t >= t_min && t <= t_max) { t_hit = t; return true; }

    // Far root
    t = (-hb + sq) / a;
    if (t >= t_min && t <= t_max) { t_hit = t; return true; }

    return false;
}

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------
struct camera
{
    vec3 eye;
    vec3 lower_left; // bottom-left of the image plane
    vec3 horizontal; // full width across the image plane
    vec3 vertical;   // full height up the image plane

    /// @brief Construct a pinhole camera.
    /// @param look_from  World position of the camera
    /// @param look_at    Target point the camera faces
    /// @param up         World-up reference vector
    /// @param vfov_deg   Vertical field of view in degrees
    /// @param aspect     Width / height ratio
    static camera make(const vec3& look_from, const vec3& look_at,
                       const vec3& up, float vfov_deg, float aspect) noexcept
    {
        const float theta = vfov_deg * 3.14159265f / 180.f;
        const float h     = std::tan(theta * 0.5f);

        const float vp_h = 2.f * h;
        const float vp_w = aspect * vp_h;

        const vec3 w = normalize(look_from - look_at);
        const vec3 u = normalize(cross(up, w));
        const vec3 v = cross(w, u);

        camera cam{};
        cam.eye        = look_from;
        cam.horizontal = u * vp_w;
        cam.vertical   = v * vp_h;
        cam.lower_left = look_from - cam.horizontal * 0.5f
                                   - cam.vertical   * 0.5f
                                   - w;
        return cam;
    }

    /// @brief Emit a ray through normalized image coordinates (s,t) ∈ [0,1].
    [[nodiscard]] ray get_ray(float s, float t) const noexcept
    {
        const vec3 target = lower_left + horizontal * s + vertical * t;
        return { eye, normalize(target - eye) };
    }
};

// ---------------------------------------------------------------------------
// Scene: sphere list + lighting + camera placement
// ---------------------------------------------------------------------------
struct scene
{
    std::vector<sphere> spheres;

    // Lighting
    vec3  light_dir{0.57735f, 0.57735f, 0.57735f}; // normalized toward light
    float ambient{0.18f};

    // Camera placement (rebuilt each render)
    vec3  camera_pos{0.f, 10.f, 10.f};
    vec3  look_at{0.f, 0.f, 0.f};
    float vfov_deg{50.f};

    /// @brief Sky gradient: lerp from white horizon to sky-blue zenith.
    [[nodiscard]] vec3 sky_color(const ray& r) const noexcept
    {
        const float t     = 0.5f * (normalize(r.dir).y + 1.f);
        const vec3  top   = {0.45f, 0.65f, 0.95f}; // sky blue
        const vec3  horiz = {1.f,   1.f,   1.f  }; // white horizon
        return horiz * (1.f - t) + top * t;
    }
};

// ---------------------------------------------------------------------------
// Path tracer – direct illumination + one specular bounce
// ---------------------------------------------------------------------------
/// @brief Trace a ray and return its radiance.
/// @param r     Ray to trace
/// @param sc    Scene description
/// @param depth Maximum remaining bounces (1 = direct-light only)
[[nodiscard]] static vec3 trace(const ray& r, const scene& sc, int depth) noexcept
{
    if (depth <= 0) return {};

    constexpr float T_MIN = 1e-3f;
    constexpr float T_MAX = 1e+6f;

    float closest_t = T_MAX;
    int   hit_idx   = -1;

    for (int i = 0; i < static_cast<int>(sc.spheres.size()); ++i)
    {
        float t{};
        if (hit_sphere(sc.spheres[i], r, T_MIN, closest_t, t))
        {
            closest_t = t;
            hit_idx   = i;
        }
    }

    if (hit_idx < 0) return sc.sky_color(r);

    const sphere& s      = sc.spheres[hit_idx];
    const vec3    hit_pt = r.at(closest_t);
    const vec3    normal = normalize(hit_pt - s.center);

    // --- Shadow test ---
    const ray shadow_ray{ hit_pt + normal * 1e-3f, sc.light_dir };
    bool in_shadow = false;
    for (const auto& other : sc.spheres)
    {
        float t{};
        if (hit_sphere(other, shadow_ray, 1e-3f, T_MAX, t))
        {
            in_shadow = true;
            break;
        }
    }

    // --- Direct diffuse shading ---
    const float diffuse   = in_shadow ? 0.f : std::max(0.f, dot(normal, sc.light_dir));
    const vec3  light_col   = {1.f, 0.98f, 0.95f}; // slightly warm white light
    const vec3  ambient_col = {sc.ambient, sc.ambient, sc.ambient};
    vec3        color       = s.albedo * (ambient_col + light_col * diffuse);

    // --- One specular bounce for shiny spheres ---
    if (depth > 1 && s.roughness < 0.95f)
    {
        const vec3 refl_dir   = reflect(r.dir, normal);
        const ray  bounce_ray = { hit_pt + normal * 1e-3f, normalize(refl_dir) };
        const vec3 bounce_col = trace(bounce_ray, sc, depth - 1);
        color += bounce_col * ((1.f - s.roughness) * 0.3f);
    }

    return color;
}

// ---------------------------------------------------------------------------
// Maze → sphere scene builder
// ---------------------------------------------------------------------------
/// @brief Parse the ASCII maze and populate the scene with spheres.
///
/// Maze characters:
///   '+'  →  corner joint   (semi-shiny blue-grey sphere)
///   '-'  →  horizontal wall (matte grey sphere)
///   '|'  →  vertical wall   (matte grey sphere)
///   ' '  →  open path       (no sphere)
///
/// @param maze_str  ASCII maze produced by MazeBuilder
/// @param sc        Scene to populate (existing spheres are cleared)
/// @param out_cx    [out] World-space X of the maze centre
/// @param out_cz    [out] World-space Z of the maze centre
static void build_scene_from_maze(const std::string& maze_str, scene& sc,
                                   float& out_cx, float& out_cz) noexcept
{
    sc.spheres.clear();

    constexpr float RADIUS = 0.36f; // sphere radius
    constexpr float SCALE  = 0.68f; // world units per ASCII character

    const vec3 wall_albedo   = {0.74f, 0.74f, 0.74f}; // light grey
    const vec3 corner_albedo = {0.52f, 0.55f, 0.68f}; // soft blue-grey
    const vec3 floor_albedo  = {0.28f, 0.52f, 0.32f}; // muted green

    int row    = 0;
    int col    = 0;
    int max_col = 0;

    for (const char ch : maze_str)
    {
        if (ch == '\n')
        {
            if (col > max_col) max_col = col;
            col = 0;
            ++row;
            continue;
        }

        if (ch == '+' || ch == '-' || ch == '|')
        {
            const bool   is_corner = (ch == '+');
            const vec3   albedo    = is_corner ? corner_albedo : wall_albedo;
            const float  rough     = is_corner ? 0.30f : 0.92f;

            sc.spheres.push_back({
                { static_cast<float>(col) * SCALE,
                  RADIUS,
                  static_cast<float>(row) * SCALE },
                RADIUS,
                albedo,
                rough
            });
        }

        ++col;
    }

    // Large floor sphere gives the appearance of a ground plane
    const float cx = static_cast<float>(max_col) * SCALE * 0.5f;
    const float cz = static_cast<float>(row)     * SCALE * 0.5f;

    sc.spheres.push_back({
        { cx, -200.f, cz },
        200.f,
        floor_albedo,
        1.0f
    });

    out_cx = cx;
    out_cz = cz;
}

// ---------------------------------------------------------------------------
// Pixel buffer helpers
// ---------------------------------------------------------------------------
[[nodiscard]] static inline uint8_t to_byte(float v) noexcept
{
    return static_cast<uint8_t>(std::clamp(v, 0.f, 1.f) * 255.f + 0.5f);
}

static inline void write_pixel(std::vector<uint8_t>& buf,
                                int x, int y, int width,
                                const vec3& color) noexcept
{
    const int idx = (y * width + x) * 4;
    buf[idx + 0] = to_byte(color.x); // R
    buf[idx + 1] = to_byte(color.y); // G
    buf[idx + 2] = to_byte(color.z); // B
    buf[idx + 3] = 255;              // A (fully opaque)
}

// ---------------------------------------------------------------------------
// Impl struct
// ---------------------------------------------------------------------------
struct pathtracer_app::pathtracer_impl
{
    // Render resolution – intentionally small for interactive performance.
    // SDL_GPU upscales this to the window size with bilinear filtering.
    static constexpr int RENDER_W = 640;
    static constexpr int RENDER_H = 360;

    // Anti-aliasing: samples per pixel (more = less noise, slower render)
    static constexpr int SAMPLES_PER_PIXEL = 4;

    // Maximum path-tracing recursion depth
    static constexpr int MAX_DEPTH = 2;

    // Maze dimensions used for scene generation
    static constexpr unsigned int MAZE_ROWS = 8;
    static constexpr unsigned int MAZE_COLS = 8;

    const std::string title;
    const std::string version;
    const int         win_w;
    const int         win_h;

    // SDL state
    SDL_Window*    window    = nullptr;
    SDL_GPUDevice* gpu_dev   = nullptr;

    // GPU texture holding the ray-traced image
    SDL_GPUTexture* render_tex = nullptr;

    // CPU pixel buffer (RGBA32) - mutable so render_scene (const) can write it
    mutable std::vector<uint8_t> pixels;

    // Dirty flag – set when the maze changes, cleared after render + upload
    mutable bool needs_render = true;

    // RNG for per-pixel jitter (anti-aliasing)
    mutable std::mt19937                          rng_engine{std::random_device{}()};
    mutable std::uniform_real_distribution<float> jitter{0.f, 1.f};

    // -----------------------------------------------------------------------
    pathtracer_impl(std::string_view t, std::string_view v, int w, int h)
        : title{t}, version{v}, win_w{w}, win_h{h}
    {
        pixels.resize(static_cast<size_t>(RENDER_W) * RENDER_H * 4, 0);
    }

    ~pathtracer_impl()
    {
        cleanup();
    }

    // -----------------------------------------------------------------------
    // SDL / GPU initialisation
    // -----------------------------------------------------------------------
    bool init() noexcept
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                         "SDL_Init failed: %s", SDL_GetError());
            return false;
        }

        const std::string win_title = title + " - " + version;
        window = SDL_CreateWindow(win_title.c_str(), win_w, win_h,
                                  SDL_WINDOW_RESIZABLE);
        if (!window)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                         "SDL_CreateWindow failed: %s", SDL_GetError());
            return false;
        }

        // Request GPU device; we ask for all common shader formats even though
        // we compile no custom shaders – SDL picks an available backend.
        gpu_dev = SDL_CreateGPUDevice(
            SDL_GPU_SHADERFORMAT_SPIRV |
            SDL_GPU_SHADERFORMAT_DXIL  |
            SDL_GPU_SHADERFORMAT_MSL,
            false,   // debug layers
            nullptr  // auto-select driver
        );

        if (!gpu_dev)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                         "SDL_CreateGPUDevice failed: %s", SDL_GetError());
            return false;
        }

        if (!SDL_ClaimWindowForGPUDevice(gpu_dev, window))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                         "SDL_ClaimWindowForGPUDevice failed: %s", SDL_GetError());
            return false;
        }

        // Allocate the GPU texture (COPY_DST so we can upload pixels to it)
        SDL_GPUTextureCreateInfo tex_info{};
        tex_info.type                 = SDL_GPU_TEXTURETYPE_2D;
        tex_info.format               = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        tex_info.usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER
                                      | SDL_GPU_TEXTUREUSAGE_COPY_DST
                                      | SDL_GPU_TEXTUREUSAGE_COPY_SRC;
        tex_info.width                = RENDER_W;
        tex_info.height               = RENDER_H;
        tex_info.layer_count_or_depth = 1;
        tex_info.num_levels           = 1;

        render_tex = SDL_CreateGPUTexture(gpu_dev, &tex_info);
        if (!render_tex)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                         "SDL_CreateGPUTexture failed: %s", SDL_GetError());
            return false;
        }

        SDL_Log("GPU driver: %s", SDL_GetGPUDeviceDriver(gpu_dev));
        return true;
    }

    void cleanup() noexcept
    {
        if (render_tex && gpu_dev)
        {
            SDL_ReleaseGPUTexture(gpu_dev, render_tex);
            render_tex = nullptr;
        }
        if (gpu_dev && window)
        {
            SDL_ReleaseWindowFromGPUDevice(gpu_dev, window);
        }
        if (gpu_dev)
        {
            SDL_DestroyGPUDevice(gpu_dev);
            gpu_dev = nullptr;
        }
        if (window)
        {
            SDL_DestroyWindow(window);
            window = nullptr;
        }
        SDL_Quit();
    }

    // -----------------------------------------------------------------------
    // Upload CPU pixel buffer → GPU texture via a transfer buffer
    // -----------------------------------------------------------------------
    void upload_pixels() const noexcept
    {
        const Uint32 buf_size = static_cast<Uint32>(RENDER_W * RENDER_H * 4);

        SDL_GPUTransferBufferCreateInfo tb_info{};
        tb_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tb_info.size  = buf_size;

        SDL_GPUTransferBuffer* transfer =
            SDL_CreateGPUTransferBuffer(gpu_dev, &tb_info);
        if (!transfer) return;

        void* mapped = SDL_MapGPUTransferBuffer(gpu_dev, transfer, false);
        if (mapped)
        {
            std::memcpy(mapped, pixels.data(), buf_size);
            SDL_UnmapGPUTransferBuffer(gpu_dev, transfer);
        }

        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu_dev);
        if (!cmd)
        {
            SDL_ReleaseGPUTransferBuffer(gpu_dev, transfer);
            return;
        }

        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

        SDL_GPUTextureTransferInfo src{};
        src.transfer_buffer = transfer;
        src.offset          = 0;
        src.pixels_per_row  = RENDER_W;
        src.rows_per_layer  = RENDER_H;

        SDL_GPUTextureRegion dst{};
        dst.texture = render_tex;
        dst.w       = RENDER_W;
        dst.h       = RENDER_H;
        dst.d       = 1;

        SDL_UploadToGPUTexture(copy_pass, &src, &dst, false);
        SDL_EndGPUCopyPass(copy_pass);

        SDL_SubmitGPUCommandBuffer(cmd);
        SDL_ReleaseGPUTransferBuffer(gpu_dev, transfer);
    }

    // -----------------------------------------------------------------------
    // Blit GPU texture → swap-chain and present
    // -----------------------------------------------------------------------
    void present_frame() const noexcept
    {
        SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(gpu_dev);
        if (!cmd) return;

        SDL_GPUTexture* swapchain = nullptr;
        Uint32          sc_w = 0, sc_h = 0;

        if (!SDL_AcquireGPUSwapchainTexture(cmd, window, &swapchain, &sc_w, &sc_h))
        {
            SDL_CancelGPUCommandBuffer(cmd);
            return;
        }

        if (swapchain && sc_w > 0 && sc_h > 0)
        {
            SDL_GPUBlitInfo blit{};
            blit.source.texture   = render_tex;
            blit.source.w         = RENDER_W;
            blit.source.h         = RENDER_H;

            blit.destination.texture = swapchain;
            blit.destination.w       = sc_w;
            blit.destination.h       = sc_h;

            blit.load_op = SDL_GPU_LOADOP_DONT_CARE;
            blit.filter  = SDL_GPU_FILTER_LINEAR; // bilinear upscale

            SDL_BlitGPUTexture(cmd, &blit);
        }

        SDL_SubmitGPUCommandBuffer(cmd);
    }

    // -----------------------------------------------------------------------
    // Software path tracer → writes into `pixels`
    // -----------------------------------------------------------------------
    void render_scene(const scene& sc) const noexcept
    {
        const float aspect = static_cast<float>(RENDER_W) /
                             static_cast<float>(RENDER_H);

        const camera cam = camera::make(
            sc.camera_pos, sc.look_at,
            {0.f, 1.f, 0.f},
            sc.vfov_deg, aspect);

        const float inv_w = 1.f / static_cast<float>(RENDER_W);
        const float inv_h = 1.f / static_cast<float>(RENDER_H);
        const float inv_s = 1.f / static_cast<float>(SAMPLES_PER_PIXEL);

        for (int y = 0; y < RENDER_H; ++y)
        {
            for (int x = 0; x < RENDER_W; ++x)
            {
                vec3 accumulated{};

                for (int s = 0; s < SAMPLES_PER_PIXEL; ++s)
                {
                    // Sub-pixel jitter for anti-aliasing
                    const float u = (static_cast<float>(x) + jitter(rng_engine)) * inv_w;
                    const float v = (static_cast<float>(RENDER_H - 1 - y) + jitter(rng_engine)) * inv_h;

                    accumulated += trace(cam.get_ray(u, v), sc, MAX_DEPTH);
                }

                // Average samples, apply square-root gamma correction (γ ≈ 2)
                vec3 col = accumulated * inv_s;
                col.x = std::sqrt(std::max(0.f, col.x));
                col.y = std::sqrt(std::max(0.f, col.y));
                col.z = std::sqrt(std::max(0.f, col.z));

                write_pixel(pixels, x, y, RENDER_W, col);
            }
        }

        needs_render = false;
    }

    // -----------------------------------------------------------------------
    // Main application loop
    // -----------------------------------------------------------------------
    bool run(mazes::randomizer& rng) noexcept
    {
        if (!init()) return false;

        scene sc;

        // Build or rebuild the maze scene
        auto regenerate_maze = [&]()
        {
            const auto algo_id = (rng(0, 1) == 0)
                                 ? mazes::algo::BINARY_TREE
                                 : mazes::algo::DFS;

            const std::string maze_str = mazes::create(
                mazes::configurator()
                    .algo_id(algo_id)
                    .rows(MAZE_ROWS)
                    .columns(MAZE_COLS)
                    .seed(rng(1u, 99999u)));

            float cx{}, cz{};
            build_scene_from_maze(maze_str, sc, cx, cz);

            // Position the camera above and in front of the maze
            const float extent =
                static_cast<float>(std::max(MAZE_ROWS, MAZE_COLS)) * 0.68f;

            sc.camera_pos = { cx, extent * 0.90f, cz + extent * 0.85f };
            sc.look_at    = { cx, 0.f, cz };
            sc.vfov_deg   = 50.f;
            sc.light_dir  = normalize({1.f, 2.f, 1.f});
            sc.ambient    = 0.18f;

            needs_render = true;
        };

        regenerate_maze();

        SDL_Log("MazePathtracer ready.");
        SDL_Log("  SPACE / R  →  new random maze");
        SDL_Log("  ESC / Q    →  quit");

        bool running = true;

#if defined(__EMSCRIPTEN__)
        EMSCRIPTEN_MAINLOOP_BEGIN
#else
        while (running)
#endif
        {
            SDL_Event ev;
            while (SDL_PollEvent(&ev))
            {
                if (ev.type == SDL_EVENT_QUIT)
                {
                    running = false;
                }
                else if (ev.type == SDL_EVENT_KEY_DOWN)
                {
                    const auto sc_code = ev.key.scancode;
                    if (sc_code == SDL_SCANCODE_ESCAPE || sc_code == SDL_SCANCODE_Q)
                    {
                        running = false;
                    }
                    else if (sc_code == SDL_SCANCODE_SPACE || sc_code == SDL_SCANCODE_R)
                    {
                        regenerate_maze();
                    }
                }
            }

            // Re-render when the maze changes (path tracing is CPU-heavy, so
            // we only render on demand and then display the cached result)
            if (needs_render)
            {
                SDL_Log("Rendering %dx%d @ %d spp ...",
                        RENDER_W, RENDER_H, SAMPLES_PER_PIXEL);

                render_scene(sc);
                upload_pixels();

                SDL_Log("Done. Press SPACE / R for a new maze.");
            }

            present_frame();

#if defined(__EMSCRIPTEN__)
        EMSCRIPTEN_MAINLOOP_END
#endif
        }

        return true;
    }
};

// ---------------------------------------------------------------------------
// pathtracer_app public interface
// ---------------------------------------------------------------------------
pathtracer_app::pathtracer_app(std::string_view title,
                               std::string_view version,
                               int w, int h)
    : m_impl{std::make_unique<pathtracer_impl>(title, version, w, h)}
{
}

pathtracer_app::pathtracer_app(const std::string& title,
                               const std::string& version,
                               int w, int h)
    : m_impl{std::make_unique<pathtracer_impl>(
          std::string_view{title}, std::string_view{version}, w, h)}
{
}

pathtracer_app::~pathtracer_app() = default;

bool pathtracer_app::run(mazes::grid_interface* /*g*/,
                         mazes::randomizer& rng) const noexcept
{
    return m_impl->run(rng);
}
