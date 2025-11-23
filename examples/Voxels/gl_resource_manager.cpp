#include "gl_resource_manager.h"
#include <SDL3/SDL.h>
#include <fstream>
#include <sstream>

namespace gl {

// ============================================================================
// GlFramebuffer Implementation
// ============================================================================

GlFramebuffer::GlFramebuffer() : m_fbo(0) {
    glGenFramebuffers(1, &m_fbo);
}

GlFramebuffer::~GlFramebuffer() {
    if (m_fbo != 0) {
        glDeleteFramebuffers(1, &m_fbo);
    }
}

GlFramebuffer::GlFramebuffer(GlFramebuffer&& other) noexcept : m_fbo(other.m_fbo) {
    other.m_fbo = 0;
}

GlFramebuffer& GlFramebuffer::operator=(GlFramebuffer&& other) noexcept {
    if (this != &other) {
        if (m_fbo != 0) {
            glDeleteFramebuffers(1, &m_fbo);
        }
        m_fbo = other.m_fbo;
        other.m_fbo = 0;
    }
    return *this;
}

void GlFramebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void GlFramebuffer::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GlFramebuffer::check_status() {
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
}

// ============================================================================
// GlTexture Implementation
// ============================================================================

GlTexture::GlTexture() : m_texture(0) {
    glGenTextures(1, &m_texture);
}

GlTexture::~GlTexture() {
    if (m_texture != 0) {
        glDeleteTextures(1, &m_texture);
    }
}

GlTexture::GlTexture(GlTexture&& other) noexcept : m_texture(other.m_texture) {
    other.m_texture = 0;
}

GlTexture& GlTexture::operator=(GlTexture&& other) noexcept {
    if (this != &other) {
        if (m_texture != 0) {
            glDeleteTextures(1, &m_texture);
        }
        m_texture = other.m_texture;
        other.m_texture = 0;
    }
    return *this;
}

void GlTexture::bind(GLenum target) const {
    glBindTexture(target, m_texture);
}

void GlTexture::unbind(GLenum target) const {
    glBindTexture(target, 0);
}

void GlTexture::set_parameter(GLenum target, GLenum pname, GLint param) {
    bind(target);
    glTexParameteri(target, pname, param);
}

void GlTexture::allocate_2d(GLsizei width, GLsizei height, GLenum internal_format,
                            GLenum format, GLenum type, const void* data) {
    bind(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, type, data);
}

void GlTexture::allocate_2d_float(GLsizei width, GLsizei height, GLenum internal_format) {
    bind(GL_TEXTURE_2D);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
}

// ============================================================================
// GlRenderbuffer Implementation
// ============================================================================

GlRenderbuffer::GlRenderbuffer() : m_rbo(0) {
    glGenRenderbuffers(1, &m_rbo);
}

GlRenderbuffer::~GlRenderbuffer() {
    if (m_rbo != 0) {
        glDeleteRenderbuffers(1, &m_rbo);
    }
}

GlRenderbuffer::GlRenderbuffer(GlRenderbuffer&& other) noexcept : m_rbo(other.m_rbo) {
    other.m_rbo = 0;
}

GlRenderbuffer& GlRenderbuffer::operator=(GlRenderbuffer&& other) noexcept {
    if (this != &other) {
        if (m_rbo != 0) {
            glDeleteRenderbuffers(1, &m_rbo);
        }
        m_rbo = other.m_rbo;
        other.m_rbo = 0;
    }
    return *this;
}

void GlRenderbuffer::bind() const {
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
}

void GlRenderbuffer::unbind() const {
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void GlRenderbuffer::allocate_storage(GLenum internal_format, GLsizei width, GLsizei height) {
    bind();
    glRenderbufferStorage(GL_RENDERBUFFER, internal_format, width, height);
}

// ============================================================================
// GlVertexArray Implementation
// ============================================================================

GlVertexArray::GlVertexArray() : m_vao(0) {
    glGenVertexArrays(1, &m_vao);
}

GlVertexArray::~GlVertexArray() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }
}

GlVertexArray::GlVertexArray(GlVertexArray&& other) noexcept : m_vao(other.m_vao) {
    other.m_vao = 0;
}

GlVertexArray& GlVertexArray::operator=(GlVertexArray&& other) noexcept {
    if (this != &other) {
        if (m_vao != 0) {
            glDeleteVertexArrays(1, &m_vao);
        }
        m_vao = other.m_vao;
        other.m_vao = 0;
    }
    return *this;
}

void GlVertexArray::bind() const {
    glBindVertexArray(m_vao);
}

void GlVertexArray::unbind() const {
    glBindVertexArray(0);
}

// ============================================================================
// GlBuffer Implementation
// ============================================================================

GlBuffer::GlBuffer() : m_buffer(0) {
    glGenBuffers(1, &m_buffer);
}

GlBuffer::~GlBuffer() {
    if (m_buffer != 0) {
        glDeleteBuffers(1, &m_buffer);
    }
}

GlBuffer::GlBuffer(GlBuffer&& other) noexcept : m_buffer(other.m_buffer) {
    other.m_buffer = 0;
}

GlBuffer& GlBuffer::operator=(GlBuffer&& other) noexcept {
    if (this != &other) {
        if (m_buffer != 0) {
            glDeleteBuffers(1, &m_buffer);
        }
        m_buffer = other.m_buffer;
        other.m_buffer = 0;
    }
    return *this;
}

void GlBuffer::bind(GLenum target) const {
    glBindBuffer(target, m_buffer);
}

void GlBuffer::unbind(GLenum target) const {
    glBindBuffer(target, 0);
}

void GlBuffer::allocate_data(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    bind(target);
    glBufferData(target, size, data, usage);
}

// ============================================================================
// GlShaderProgram Implementation
// ============================================================================

GlShaderProgram::GlShaderProgram() : m_program(0) {
    m_program = glCreateProgram();
}

GlShaderProgram::~GlShaderProgram() {
    if (m_program != 0) {
        glDeleteProgram(m_program);
    }
}

GlShaderProgram::GlShaderProgram(GlShaderProgram&& other) noexcept
    : m_program(other.m_program), m_uniform_cache(std::move(other.m_uniform_cache)) {
    other.m_program = 0;
}

GlShaderProgram& GlShaderProgram::operator=(GlShaderProgram&& other) noexcept {
    if (this != &other) {
        if (m_program != 0) {
            glDeleteProgram(m_program);
        }
        m_program = other.m_program;
        m_uniform_cache = std::move(other.m_uniform_cache);
        other.m_program = 0;
    }
    return *this;
}

void GlShaderProgram::use() const {
    glUseProgram(m_program);
}

bool GlShaderProgram::load_from_files(const std::string& vertex_path, const std::string& fragment_path) {
    // Read vertex shader
    std::ifstream vshader_file(vertex_path);
    if (!vshader_file.is_open()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to open vertex shader: %s\n", vertex_path.c_str());
        return false;
    }
    std::stringstream vshader_stream;
    vshader_stream << vshader_file.rdbuf();
    std::string vertex_source = vshader_stream.str();

    // Read fragment shader
    std::ifstream fshader_file(fragment_path);
    if (!fshader_file.is_open()) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to open fragment shader: %s\n", fragment_path.c_str());
        return false;
    }
    std::stringstream fshader_stream;
    fshader_stream << fshader_file.rdbuf();
    std::string fragment_source = fshader_stream.str();

    // Compile shaders
    GLuint vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source);
    if (vertex_shader == 0) return false;

    GLuint fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    if (fragment_shader == 0) {
        glDeleteShader(vertex_shader);
        return false;
    }

    // Link program
    bool success = link_program(vertex_shader, fragment_shader);

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return success;
}

GLuint GlShaderProgram::compile_shader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Shader compilation failed: %s\n", info_log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

bool GlShaderProgram::link_program(GLuint vertex_shader, GLuint fragment_shader) {
    glAttachShader(m_program, vertex_shader);
    glAttachShader(m_program, fragment_shader);
    glLinkProgram(m_program);

    GLint success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(m_program, 512, nullptr, info_log);
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Shader program linking failed: %s\n", info_log);
        return false;
    }

    return true;
}

GLint GlShaderProgram::get_uniform_location(const std::string& name) {
    auto it = m_uniform_cache.find(name);
    if (it != m_uniform_cache.end()) {
        return it->second;
    }

    GLint location = glGetUniformLocation(m_program, name.c_str());
    m_uniform_cache[name] = location;
    return location;
}

void GlShaderProgram::set_uniform(const std::string& name, GLint value) {
    glUniform1i(get_uniform_location(name), value);
}

void GlShaderProgram::set_uniform(const std::string& name, GLfloat value) {
    glUniform1f(get_uniform_location(name), value);
}

void GlShaderProgram::set_uniform(const std::string& name, GLfloat v0, GLfloat v1, GLfloat v2) {
    glUniform3f(get_uniform_location(name), v0, v1, v2);
}

void GlShaderProgram::set_uniform_matrix4fv(const std::string& name, const GLfloat* value) {
    glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, value);
}

} // namespace gl

