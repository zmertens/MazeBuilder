#include "shader.h"

#include <string>

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>

#include "sdl_gl_helper.h"

shader::shader(shader &&other) noexcept : m_program(0)
{
}

shader &shader::operator=(shader &&other) noexcept
{
    if (this != &other)
    {

    }
    return *this;
}

shader::~shader() noexcept
{
    if (m_program)
    {
        glDeleteProgram(m_program);
        m_program = 0;
    }
}

std::uint32_t shader::make_shader(const std::string_view sources, std::string_view path)
{
    auto convert_id_to_gl_enum = [&path]() -> GLenum
    {
        if (path.find("vertex") != std::string_view::npos) {
            return GL_VERTEX_SHADER;
        }
        if (path.find("fragment") != std::string_view::npos) {
            return GL_FRAGMENT_SHADER;
        }

        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Unknown shader type in path: %s", path.data());
        return 0;
    };

    const GLuint shader = glCreateShader(convert_id_to_gl_enum());
    const GLchar* source_ptr = sources.data();
    glShaderSource(shader, 1, &source_ptr, nullptr);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        const auto info = static_cast<GLchar*>(SDL_calloc(length, sizeof(GLchar)));
        glGetShaderInfoLog(shader, length, nullptr, info);
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "glCompileShader failed:\n%s\n", info);
        SDL_free(info);
    }
    return shader;
}

std::uint32_t shader::load_shader(std::string_view path)
{
    auto data = sdl_gl_helper::load_file_to_string(path);
    const GLuint result = make_shader(data, path);
    return result;
}

std::uint32_t shader::make_program(const std::uint32_t shader1, const std::uint32_t shader2)
{
    GLuint program = glCreateProgram();
    glAttachShader(program, shader1);
    glAttachShader(program, shader2);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        const auto info = static_cast<GLchar*>(SDL_calloc(length, sizeof(GLchar)));
        glGetProgramInfoLog(program, length, nullptr, info);
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "glLinkProgram failed: %s\n", info);
        SDL_free(info);
    }
    glDetachShader(program, shader1);
    glDetachShader(program, shader2);
    glDeleteShader(shader1);
    glDeleteShader(shader2);
    return program;
}

std::uint32_t shader::load_program(std::string_view vertex_shader_path, std::string_view fragment_shader_path)
{
    const GLuint shader1 = load_shader(vertex_shader_path);
    const GLuint shader2 = load_shader(fragment_shader_path);
    const GLuint program = make_program(shader1, shader2);
    this->m_program = program;

    return program;
}

std::uint32_t shader::get() const noexcept
{
    return this->m_program;
}
