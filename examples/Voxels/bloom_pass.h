#ifndef BLOOM_PASS_H
#define BLOOM_PASS_H

#include <array>
#include <cstdint>

// Gaussian bloom compositor inspired by OGRE3D's Compositor system.
//
// Rendering flow (three logical passes):
//   1. bind_scene()         — bind scene MRT FBO; caller renders all geometry here.
//   2. set_mrt_mode()       — enable both colour attachments (scene + bright extraction).
//      set_single_mode()    — enable only attachment 0 (sky, UI, wireframe — no bloom).
//   3. execute()            — blur bright pass × BLUR_PASSES*2 (ping-pong), composite to screen.
//
// Initialise once in world::init() after shaders are loaded.
// Call resize() if the window is ever resized.
class bloom_pass
{
public:
    // Number of horizontal+vertical blur iterations.  Higher = wider, softer bloom.
    static constexpr int BLUR_PASSES = 5;
    static constexpr float BLOOM_STRENGTH = 0.6f;

    [[nodiscard]] bool init(int width, int height,
                            std::uint32_t blur_program,
                            std::uint32_t composite_program) noexcept;

    // Bind scene MRT FBO and activate both draw buffers.
    void bind_scene() const noexcept;

    // Switch to MRT mode (colour attachment 0 + 1) for bloom-eligible geometry.
    void set_mrt_mode() const noexcept;

    // Switch to single-buffer mode (colour attachment 0 only) for sky and UI.
    void set_single_mode() const noexcept;

    // Run blur passes then composite scene + bloom to the default framebuffer.
    // Pass strength=0 to skip blur and composite the scene without any bloom.
    void execute(float strength = BLOOM_STRENGTH) const noexcept;

    // Recreate FBOs at a new resolution (call on window resize).
    void resize(int new_width, int new_height) noexcept;

    void destroy() noexcept;

private:
    struct components final
    {
        static constexpr int MAX_COLOR_ATTACHMENTS = 2;

        std::uint32_t fbo{};
        std::array<std::uint32_t, MAX_COLOR_ATTACHMENTS> color_tex{};
        std::uint32_t depth_rbo{};
        int num_color_attachments{1};

        components();
        ~components() noexcept;
        components &operator=(const components &) = delete;
        components &operator=(components &&) noexcept = delete;
        components(const components &) = delete;
        components(components &&) noexcept = delete;

        [[nodiscard]] bool init(int w, int h,
                                int color_attachments = 1,
                                bool depth = true) noexcept;
    };

    void create_fbos(int w, int h) noexcept;
    void destroy_fbos() noexcept;
    void create_screen_quad() noexcept;

    // Pass 1 — MRT scene FBO:
    //   color_tex[0] = full scene colour
    //   color_tex[1] = pixels above brightness threshold (bloom candidates)
    components m_scene_pass{};

    // Passes 2-3 — ping-pong Gaussian blur on the bright texture.
    std::array<components, 2> m_blur_pass{};

    // Shared full-screen triangle-strip quad (NDC positions + UVs).
    std::uint32_t m_quad_vao{};
    std::uint32_t m_quad_vbo{};

    // Cached GL program handles.
    std::uint32_t m_blur_program{};
    std::uint32_t m_composite_program{};

    // Cached uniform locations (set during init to avoid per-frame queries).
    std::uint32_t m_blur_loc_horizontal{-1u};
    std::uint32_t m_blur_loc_image{-1u};
    std::uint32_t m_comp_loc_scene{-1u};
    std::uint32_t m_comp_loc_bloom{-1u};
    std::uint32_t m_comp_loc_strength{-1u};

    std::uint32_t m_width{};
    std::uint32_t m_height{};
};

#endif // BLOOM_PASS_H
