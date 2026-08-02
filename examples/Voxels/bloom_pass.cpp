#include "bloom_pass.h"

#include <SDL3/SDL.h>

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

// ---------------------------------------------------------------------------
// render_pass implementation
// ---------------------------------------------------------------------------

bool bloom_pass::components::init(const int w, const int h,
                                  const int color_attachments,
                                  const bool depth) noexcept
{
    // width and height members removed
    num_color_attachments = color_attachments;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    GLenum draw_buffers[MAX_COLOR_ATTACHMENTS]{};
    for (int i = 0; i < color_attachments; ++i)
    {
        glGenTextures(1, &color_tex.at(static_cast<std::size_t>(i)));
        glBindTexture(GL_TEXTURE_2D, color_tex.at(static_cast<std::size_t>(i)));
        // GL_RGBA16F allows values > 1.0, which is required for the HDR bloom threshold.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i),
                               GL_TEXTURE_2D,
                               color_tex.at(static_cast<std::size_t>(i)), 0);
        draw_buffers[i] = static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i);
    }
    glDrawBuffers(color_attachments, draw_buffers);

    if (depth)
    {
        glGenRenderbuffers(1, &depth_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, depth_rbo);
    }

    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return complete;
}

bloom_pass::components::components()
: fbo(0),
  color_tex{},
  depth_rbo(0),
  num_color_attachments(1)
{

}

bloom_pass::components::~components() noexcept
{
    for (int i = 0; i < num_color_attachments; ++i)
    {
        if (color_tex.at(static_cast<std::size_t>(i)))
        {
            glDeleteTextures(1, &color_tex.at(static_cast<std::size_t>(i)));
            color_tex.at(static_cast<std::size_t>(i)) = 0;
        }
    }
    if (depth_rbo)
    {
        glDeleteRenderbuffers(1, &depth_rbo);
        depth_rbo = 0;
    }
    if (fbo)
    {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
}

// ---------------------------------------------------------------------------
// bloom_pass implementation
// ---------------------------------------------------------------------------

bool bloom_pass::init(const int width, const int height,
                      const std::uint32_t blur_program,
                      const std::uint32_t composite_program) noexcept
{
    m_blur_program = blur_program;
    m_composite_program = composite_program;

    // Cache uniform locations to avoid per-frame queries.
    m_blur_loc_horizontal = glGetUniformLocation(blur_program, "horizontal");
    m_blur_loc_image = glGetUniformLocation(blur_program, "image");
    m_comp_loc_scene = glGetUniformLocation(composite_program, "scene");
    m_comp_loc_bloom = glGetUniformLocation(composite_program, "bloom_blur");
    m_comp_loc_strength = glGetUniformLocation(composite_program, "bloom_strength");

    create_fbos(width, height);
    create_screen_quad();

    return m_scene_pass.fbo != 0 && m_blur_pass.at(0).fbo != 0 && m_blur_pass.at(1).fbo != 0;
}

void bloom_pass::create_fbos(const int w, const int h) noexcept
{
    m_width = w;
    m_height = h;

    // MRT scene pass: attachment 0 = full scene, attachment 1 = bright pixels.
    if (!m_scene_pass.init(w, h, 2, true))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "bloom_pass: scene MRT FBO incomplete\n");
    }

    // Two ping-pong blur targets — no depth needed for full-screen passes.
    for (auto &pass : m_blur_pass)
    {
        if (!pass.init(w, h, 1, false))
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "bloom_pass: blur FBO incomplete\n");
        }
    }
}

void bloom_pass::create_screen_quad() noexcept
{
    // Triangle strip covering the entire clip space, with UVs [0,1].
    static constexpr float quad_verts[] = {
        -1.f,
        -1.f,
        0.f,
        0.f,
        1.f,
        -1.f,
        1.f,
        0.f,
        -1.f,
        1.f,
        0.f,
        1.f,
        1.f,
        1.f,
        1.f,
        1.f,
    };

    glGenVertexArrays(1, &m_quad_vao);
    glGenBuffers(1, &m_quad_vbo);

    glBindVertexArray(m_quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);

    // layout(location = 0) vec2 position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<const void *>(0));
    // layout(location = 1) vec2 uv
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<const void *>(2 * sizeof(float)));

    glBindVertexArray(0);
}

void bloom_pass::destroy_fbos() noexcept
{
    m_scene_pass.~components();
    for (auto &pass : m_blur_pass)
    {
        pass.~components();
    }
}

void bloom_pass::bind_scene() const noexcept
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_scene_pass.fbo);
    // Activate both draw buffers so the block shader can write to both.
    // Switched per sub-pass from world::draw() via set_mrt_mode / set_single_mode.
    constexpr GLenum both[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, both);
}

void bloom_pass::set_mrt_mode() const noexcept
{
    constexpr GLenum both[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, both);
}

void bloom_pass::set_single_mode() const noexcept
{
    constexpr GLenum single[] = {GL_COLOR_ATTACHMENT0, GL_NONE};
    glDrawBuffers(2, single);
}

void bloom_pass::execute(const float strength) const noexcept
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // When strength is 0 (bloom disabled) skip the blur passes entirely.
    GLuint src_tex = m_scene_pass.color_tex.at(1);

    if (strength > 0.0f)
    {
        // --- Gaussian blur on the bright attachment (ping-pong) ---
        glUseProgram(m_blur_program);
        glUniform1i(m_blur_loc_image, 0);

        bool horizontal = true;

        for (int i = 0; i < BLUR_PASSES * 2; ++i)
        {
            const auto dst_idx = static_cast<std::size_t>(horizontal ? 0 : 1);
            glBindFramebuffer(GL_FRAMEBUFFER, m_blur_pass.at(dst_idx).fbo);
            glClear(GL_COLOR_BUFFER_BIT);

            glUniform1i(m_blur_loc_horizontal, static_cast<GLint>(horizontal));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, src_tex);

            glBindVertexArray(m_quad_vao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            src_tex = m_blur_pass.at(dst_idx).color_tex.at(0);
            horizontal = !horizontal;
        }
        // src_tex now holds the final blurred bright texture.
    }

    // --- Composite: scene + blurred bloom → default framebuffer ---
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(m_composite_program);
    glUniform1i(m_comp_loc_scene, 0);
    glUniform1i(m_comp_loc_bloom, 1);
    glUniform1f(m_comp_loc_strength, strength);

    // Full scene render
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_scene_pass.color_tex.at(0));
    glActiveTexture(GL_TEXTURE1);
    // blurred bright pass
    glBindTexture(GL_TEXTURE_2D, src_tex);

    glBindVertexArray(m_quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    // Restore GL state for the next frame's geometry pass.
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

void bloom_pass::resize(const int new_width, const int new_height) noexcept
{
    destroy_fbos();
    create_fbos(new_width, new_height);
}

void bloom_pass::destroy() noexcept
{
    destroy_fbos();
    if (m_quad_vao)
    {
        glDeleteVertexArrays(1, &m_quad_vao);
        m_quad_vao = 0;
    }
    if (m_quad_vbo)
    {
        glDeleteBuffers(1, &m_quad_vbo);
        m_quad_vbo = 0;
    }
}
