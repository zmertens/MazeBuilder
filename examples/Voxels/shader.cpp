#include "shader.h"

#include <string>

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>

shader::shader(shader &&other) noexcept
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

}

std::uint32_t shader::make_shader(std::string_view sources, std::string_view path)
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

    auto sdl_file_io = [&path]()->std::string
    {
        // Open binary file
        SDL_IOStream* io = SDL_IOFromFile(path.data(), "r");
        if (io == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_IOFromFile failed: %s", SDL_GetError());
            return "";
        }
        const auto data_size = SDL_GetIOSize(io);
        // Allocate memory for the file content + null terminator
        const auto data = static_cast<char*>(SDL_malloc(data_size + 1));

        if (data == nullptr) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_malloc failed: %s", SDL_GetError());
            SDL_CloseIO(io);
            return "";
        }

        // Read file into memory
        // SDL_ReadIO returns the number of bytes read, or 0 on error or end of file
        int nb_read_total = 0, nb_read_size = 1;
        auto buf = data;
        while (nb_read_total < data_size && nb_read_size != 0) {
            nb_read_size = SDL_ReadIO(io, buf, (data_size - nb_read_total));
            nb_read_total += nb_read_size;
            buf += nb_read_size;
        }

        SDL_CloseIO(io);
        if (nb_read_total != data_size) {
            SDL_free(data);
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to read complete file: %s", SDL_GetError());
            return "";
        }
        data[nb_read_total] = '\0';
        std::string string_data(data, nb_read_total);
        SDL_free(data);
        return string_data;
    };

    auto data = sdl_file_io();
    const GLuint result = make_shader(data, path);
    return result;
}

std::uint32_t shader::make_program(std::uint32_t shader1, std::uint32_t shader2)
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
        GLchar *info = (GLchar*) calloc(length, sizeof(GLchar));
        glGetProgramInfoLog(program, length, NULL, info);
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
    GLuint shader2 = load_shader(fragment_shader_path);
    GLuint program = make_program(shader1, shader2);
    this->m_program = program;

    return program;
}

std::uint32_t shader::get() const noexcept
{
    return this->m_program;
}
