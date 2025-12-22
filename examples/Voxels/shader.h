#ifndef SHADER_H
#define SHADER_H

#include <cstdint>
#include <string_view>

/// @file shader.h
/// @brief shader class for SDL3/OpenGL
/// @details This class wraps OpenGL shader requirements
class shader
{
public:
    shader() = default;

    ~shader() noexcept;

    shader(const shader&) = delete;
    shader& operator=(const shader&) = delete;

    shader(shader&& other) noexcept;
    shader& operator=(shader&& other) noexcept;

    static std::uint32_t make_shader(std::string_view sources, std::string_view path);
    static std::uint32_t load_shader(std::string_view path);
    static std::uint32_t make_program(std::uint32_t shader1, std::uint32_t shader2);
    std::uint32_t load_program(std::string_view vertex_shader_path, std::string_view fragment_shader_path);

    [[nodiscard]] std::uint32_t get() const noexcept;

private:
    std::uint32_t m_program;
}; // shader class

#endif // SHADER_H
