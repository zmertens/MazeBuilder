#ifndef GL_TYPES_H
#define GL_TYPES_H

#include <cstdint>

// OpenGL primitive type aliases - abstract away OpenGL dependencies
using gl_unsigned_int = std::uint32_t;
using gl_int = std::int32_t;
using gl_float = float;
using gl_sizei = std::int32_t;
using gl_enum = std::uint32_t;
using gl_boolean = std::uint8_t;
using gl_bitfield = std::uint32_t;
using gl_byte = std::int8_t;
using gl_ubyte = std::uint8_t;
using gl_short = std::int16_t;
using gl_ushort = std::uint16_t;
using gl_double = double;
using gl_void = void;

// OpenGL handle types
using gl_buffer_handle = gl_unsigned_int;
using gl_texture_handle = gl_unsigned_int;
using gl_program_handle = gl_unsigned_int;
using gl_shader_handle = gl_unsigned_int;
using gl_framebuffer_handle = gl_unsigned_int;
using gl_renderbuffer_handle = gl_unsigned_int;
using gl_vertex_array_handle = gl_unsigned_int;

// OpenGL constants as constexpr (to be defined based on actual GL constants)
namespace gl_constants {
    constexpr gl_enum TRIANGLES = 0x0004;
    constexpr gl_enum LINES = 0x0001;
    constexpr gl_enum FLOAT = 0x1406;
    constexpr gl_enum UNSIGNED_BYTE = 0x1401;
    constexpr gl_enum TEXTURE_2D = 0x0DE1;
    constexpr gl_enum TEXTURE_CUBE_MAP = 0x8513;
    constexpr gl_enum ARRAY_BUFFER = 0x8892;
    constexpr gl_enum STATIC_DRAW = 0x88E4;
    constexpr gl_enum FRAMEBUFFER = 0x8D40;
    constexpr gl_enum RENDERBUFFER = 0x8D41;
    // ... other constants as needed
}

#endif // GL_TYPES_H
