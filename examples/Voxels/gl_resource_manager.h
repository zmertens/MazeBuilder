#ifndef GL_RESOURCE_MANAGER_H
#define GL_RESOURCE_MANAGER_H

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace gl {

/// @brief RAII wrapper for OpenGL framebuffer objects
class GlFramebuffer {
public:
    GlFramebuffer();
    ~GlFramebuffer();

    // Non-copyable
    GlFramebuffer(const GlFramebuffer&) = delete;
    GlFramebuffer& operator=(const GlFramebuffer&) = delete;

    // Movable
    GlFramebuffer(GlFramebuffer&& other) noexcept;
    GlFramebuffer& operator=(GlFramebuffer&& other) noexcept;

    void bind() const;
    void unbind() const;
    GLuint get() const { return m_fbo; }
    bool is_valid() const { return m_fbo != 0; }

    static void check_status();

private:
    GLuint m_fbo;
};

/// @brief RAII wrapper for OpenGL texture objects
class GlTexture {
public:
    GlTexture();
    ~GlTexture();

    // Non-copyable
    GlTexture(const GlTexture&) = delete;
    GlTexture& operator=(const GlTexture&) = delete;

    // Movable
    GlTexture(GlTexture&& other) noexcept;
    GlTexture& operator=(GlTexture&& other) noexcept;

    void bind(GLenum target = GL_TEXTURE_2D) const;
    void unbind(GLenum target = GL_TEXTURE_2D) const;
    GLuint get() const { return m_texture; }
    bool is_valid() const { return m_texture != 0; }

    void set_parameter(GLenum target, GLenum pname, GLint param);
    void allocate_2d(GLsizei width, GLsizei height, GLenum internal_format,
                     GLenum format, GLenum type, const void* data = nullptr);
    void allocate_2d_float(GLsizei width, GLsizei height, GLenum internal_format);

private:
    GLuint m_texture;
};

/// @brief RAII wrapper for OpenGL renderbuffer objects
class GlRenderbuffer {
public:
    GlRenderbuffer();
    ~GlRenderbuffer();

    // Non-copyable
    GlRenderbuffer(const GlRenderbuffer&) = delete;
    GlRenderbuffer& operator=(const GlRenderbuffer&) = delete;

    // Movable
    GlRenderbuffer(GlRenderbuffer&& other) noexcept;
    GlRenderbuffer& operator=(GlRenderbuffer&& other) noexcept;

    void bind() const;
    void unbind() const;
    GLuint get() const { return m_rbo; }
    bool is_valid() const { return m_rbo != 0; }

    void allocate_storage(GLenum internal_format, GLsizei width, GLsizei height);

private:
    GLuint m_rbo;
};

/// @brief RAII wrapper for OpenGL vertex array objects
class GlVertexArray {
public:
    GlVertexArray();
    ~GlVertexArray();

    // Non-copyable
    GlVertexArray(const GlVertexArray&) = delete;
    GlVertexArray& operator=(const GlVertexArray&) = delete;

    // Movable
    GlVertexArray(GlVertexArray&& other) noexcept;
    GlVertexArray& operator=(GlVertexArray&& other) noexcept;

    void bind() const;
    void unbind() const;
    GLuint get() const { return m_vao; }
    bool is_valid() const { return m_vao != 0; }

private:
    GLuint m_vao;
};

/// @brief RAII wrapper for OpenGL buffer objects
class GlBuffer {
public:
    GlBuffer();
    ~GlBuffer();

    // Non-copyable
    GlBuffer(const GlBuffer&) = delete;
    GlBuffer& operator=(const GlBuffer&) = delete;

    // Movable
    GlBuffer(GlBuffer&& other) noexcept;
    GlBuffer& operator=(GlBuffer&& other) noexcept;

    void bind(GLenum target = GL_ARRAY_BUFFER) const;
    void unbind(GLenum target = GL_ARRAY_BUFFER) const;
    GLuint get() const { return m_buffer; }
    bool is_valid() const { return m_buffer != 0; }

    void allocate_data(GLenum target, GLsizeiptr size, const void* data, GLenum usage);

private:
    GLuint m_buffer;
};

/// @brief RAII wrapper for OpenGL shader program objects
class GlShaderProgram {
public:
    GlShaderProgram();
    ~GlShaderProgram();

    // Non-copyable
    GlShaderProgram(const GlShaderProgram&) = delete;
    GlShaderProgram& operator=(const GlShaderProgram&) = delete;

    // Movable
    GlShaderProgram(GlShaderProgram&& other) noexcept;
    GlShaderProgram& operator=(GlShaderProgram&& other) noexcept;

    void use() const;
    GLuint get() const { return m_program; }
    bool is_valid() const { return m_program != 0; }

    // Load and compile shaders from files
    bool load_from_files(const std::string& vertex_path, const std::string& fragment_path);

    // Uniform methods with caching
    GLint get_uniform_location(const std::string& name);
    void set_uniform(const std::string& name, GLint value);
    void set_uniform(const std::string& name, GLfloat value);
    void set_uniform(const std::string& name, GLfloat v0, GLfloat v1, GLfloat v2);
    void set_uniform_matrix4fv(const std::string& name, const GLfloat* value);

private:
    GLuint m_program;
    std::unordered_map<std::string, GLint> m_uniform_cache;

    GLuint compile_shader(GLenum type, const std::string& source);
    bool link_program(GLuint vertex_shader, GLuint fragment_shader);
};

} // namespace gl

#endif // GL_RESOURCE_MANAGER_H

