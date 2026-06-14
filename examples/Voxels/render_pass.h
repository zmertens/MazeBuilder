#pragma once

#include <array>
#include <cstdint>

// RAII framebuffer object — the "RenderTarget" concept from OGRE3D.
// Owns up to MAX_COLOR_ATTACHMENTS color textures plus an optional depth renderbuffer.
// All GL calls live in bloom_pass.cpp; this header stays GL-free so any .h can include it cheaply.
struct render_pass
{
    static constexpr int MAX_COLOR_ATTACHMENTS = 2;

    std::uint32_t fbo{};
    std::array<std::uint32_t, MAX_COLOR_ATTACHMENTS> color_tex{};
    std::uint32_t depth_rbo{};
    int width{};
    int height{};
    int num_color_attachments{1};

    // Allocate and complete the FBO.
    // color_attachments: 1 for a standard pass, 2 for MRT (scene + bright extraction).
    // depth: allocate a depth renderbuffer for geometry passes.
    [[nodiscard]] bool init(int w, int h,
                            int color_attachments = 1,
                            bool depth = true) noexcept;

    void bind() const noexcept;
    static void unbind() noexcept;  // binds FBO 0 (default framebuffer)
    void destroy() noexcept;
};
