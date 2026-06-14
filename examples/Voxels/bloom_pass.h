#pragma once

#include <array>
#include <cstdint>

#include "render_pass.h"

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
    static constexpr int   BLUR_PASSES    = 5;
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

    // Free all GL resources.  Safe to call even if init() was never called.
    void destroy() noexcept;

private:
    void create_fbos(int w, int h) noexcept;
    void destroy_fbos() noexcept;
    void create_screen_quad() noexcept;

    // Pass 1 — MRT scene FBO:
    //   color_tex[0] = full scene colour
    //   color_tex[1] = pixels above brightness threshold (bloom candidates)
    render_pass m_scene_pass{};

    // Passes 2-3 — ping-pong Gaussian blur on the bright texture.
    std::array<render_pass, 2> m_blur_pass{};

    // Shared full-screen triangle-strip quad (NDC positions + UVs).
    std::uint32_t m_quad_vao{};
    std::uint32_t m_quad_vbo{};

    // Cached GL program handles.
    std::uint32_t m_blur_program{};
    std::uint32_t m_composite_program{};

    // Cached uniform locations (set during init to avoid per-frame queries).
    int m_blur_loc_horizontal{-1};
    int m_blur_loc_image{-1};
    int m_comp_loc_scene{-1};
    int m_comp_loc_bloom{-1};
    int m_comp_loc_strength{-1};

    int m_width{};
    int m_height{};
};
