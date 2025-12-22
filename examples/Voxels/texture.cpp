#include "texture.h"

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>

#include "sdl_gl_helper.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <vector>
#include <algorithm>

namespace
{
    /// Create a rotated copy of RGBA texture data (180 degrees)
    std::vector<std::uint8_t> create_rotated_180(const std::uint8_t* data, int width, int height) noexcept
    {
        if (data == nullptr || width <= 0 || height <= 0)
        {
            return {};
        }

        const size_t total_bytes = width * height * 4;
        std::vector<std::uint8_t> rotated(total_bytes);

        const int total_pixels = width * height;

        // Copy pixels in reverse order
        for (int i = 0; i < total_pixels; ++i)
        {
            const int src_idx = i * 4;
            const int dst_idx = (total_pixels - 1 - i) * 4;

            rotated[dst_idx + 0] = data[src_idx + 0]; // R
            rotated[dst_idx + 1] = data[src_idx + 1]; // G
            rotated[dst_idx + 2] = data[src_idx + 2]; // B
            rotated[dst_idx + 3] = data[src_idx + 3]; // A
        }

        return rotated;
    }
} // anonymous namespace

texture::texture(texture&& other) noexcept
{
    m_texture = other.m_texture;
    m_width = other.m_width;
    m_height = other.m_height;
    m_pixel_data = other.m_pixel_data;

    other.m_texture = 0;
    other.m_width = 0;
    other.m_height = 0;
    other.m_pixel_data = nullptr;
}

texture& texture::operator=(texture&& other) noexcept
{
    if (this != &other)
    {
        free();

        m_texture = other.m_texture;
        m_width = other.m_width;
        m_height = other.m_height;
        m_pixel_data = other.m_pixel_data;

        other.m_texture = 0;
        other.m_width = 0;
        other.m_height = 0;
        other.m_pixel_data = nullptr;
    }
    return *this;
}

texture::~texture() noexcept
{
    this->free();
}

void texture::free() noexcept
{
    if (m_texture != 0)
    {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
        m_width = 0;
        m_height = 0;
        m_pixel_data = nullptr;
    }
}

std::uint32_t texture::get() const noexcept
{
    return this->m_texture;
}

int texture::get_width() const noexcept
{
    return this->m_width;
}

int texture::get_height() const noexcept
{
    return this->m_height;
}

std::uint8_t* texture::get_pixel_data() const noexcept
{
    return this->m_pixel_data;
}

// Load an image file using stb_image and create an SDL texture
bool texture::load_from_file(const std::string_view filepath, const std::uint32_t channel_offset) noexcept
{
    this->free();

    stbi_set_flip_vertically_on_load(true);

    int width, height;
    // n stores number of components (channels)
    int n;

    auto buffer = sdl_gl_helper::load_file_binary(filepath);
    if (buffer.empty())
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load file %s into memory\n", filepath.data());
        return false;
    }

    // Force RGBA (4 components) for consistency
    auto* data = stbi_load_from_memory(buffer.data(), static_cast<int>(buffer.size()),
                                       &width, &height, &n, 4);

    if (data == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "stbi_load %s failed: %s\n",
                     filepath.data(), stbi_failure_reason());
        return false;
    }

    glGenTextures(1, &m_texture);
    glActiveTexture(GL_TEXTURE0 + channel_offset);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload texture data - now we know it's always RGBA
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data);

    // Generate mipmaps for better quality
    glGenerateMipmap(GL_TEXTURE_2D);

    // Check for OpenGL errors
    if (const GLenum error = glGetError(); error != GL_NO_ERROR)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "OpenGL error after loading %s: 0x%x\n",
                     filepath.data(), error);
        stbi_image_free(data);
        return false;
    }

    m_width = width;
    m_height = height;
    m_pixel_data = data;
    stbi_image_free(data);

    return true;
}

bool texture::load_target(const int w, const int h) noexcept
{
    this->free();

    m_width = w;
    m_height = h;

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create empty texture for render target
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    if (const GLenum error = glGetError(); error != GL_NO_ERROR)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "OpenGL error creating render target %dx%d: 0x%x\n", w, h, error);
        return false;
    }

    return true;
}

/// Load texture from raw RGBA memory data
/// @param data RGBA pixel data
/// @param width texture width
/// @param height texture height
/// @param channel_offset texture unit offset (default 0)
/// @param rotate_180 if true, rotate texture 180 degrees (default false)
bool texture::load_from_memory(const std::uint8_t* data, const int width, const int height,
                               const std::uint32_t channel_offset, const bool rotate_180) noexcept
{
    if (data == nullptr || width <= 0 || height <= 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid parameters for load_from_memory\n");
        return false;
    }

    this->free();

    // If rotation requested, create rotated copy
    const std::uint8_t* upload_data = data;
    std::vector<std::uint8_t> rotated_buffer;

    if (rotate_180)
    {
        rotated_buffer = create_rotated_180(data, width, height);
        if (rotated_buffer.empty())
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create rotated texture copy\n");
            return false;
        }
        upload_data = rotated_buffer.data();
    }

    this->m_pixel_data = const_cast<std::uint8_t*>(data);

    glGenTextures(1, &m_texture);
    glActiveTexture(GL_TEXTURE0 + channel_offset);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload texture data - RGBA format (potentially rotated)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, upload_data);

    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    // Check for OpenGL errors
    if (const GLenum error = glGetError(); error != GL_NO_ERROR)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "OpenGL error after loading from memory: 0x%x\n", error);
        return false;
    }

    m_width = width;
    m_height = height;

    return true;
}

/// Update texture from raw RGBA memory data efficiently
/// Uses glTexSubImage2D when dimensions match, otherwise reallocates
/// @param data RGBA pixel data
/// @param width new width
/// @param height new height
/// @param channel_offset texture unit offset
/// @param rotate_180 if true, rotate texture 180 degrees (default false)
bool texture::update_from_memory(const std::uint8_t* data, const int width, const int height,
                                 const std::uint32_t channel_offset, const bool rotate_180) noexcept
{
    if (data == nullptr || width <= 0 || height <= 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid parameters for update_from_memory\n");
        return false;
    }

    // Enforce size constraints
    if (width > MAX_TEXTURE_WIDTH || height > MAX_TEXTURE_HEIGHT)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                     "Texture dimensions %dx%d exceed maximum %dx%d\n",
                     width, height, MAX_TEXTURE_WIDTH, MAX_TEXTURE_HEIGHT);
        return false;
    }

    m_pixel_data = const_cast<std::uint8_t*>(data);

    // If texture doesn't exist or dimensions changed, reallocate
    if (m_texture == 0 || m_width != width || m_height != height)
    {
        SDL_Log("Reallocating texture: %dx%d -> %dx%d\n", m_width, m_height, width, height);
        return load_from_memory(data, width, height, channel_offset, rotate_180);
    }

    // If rotation requested, create rotated copy
    const std::uint8_t* upload_data = data;
    std::vector<std::uint8_t> rotated_buffer;

    if (rotate_180)
    {
        rotated_buffer = create_rotated_180(data, width, height);
        if (rotated_buffer.empty())
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create rotated texture copy\n");
            return false;
        }
        upload_data = rotated_buffer.data();
    }

    // Efficient update using glTexSubImage2D (reuses existing texture)
    glActiveTexture(GL_TEXTURE0 + channel_offset);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA,
                    GL_UNSIGNED_BYTE, upload_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (const GLenum error = glGetError(); error != GL_NO_ERROR)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "OpenGL error in update_from_memory: 0x%x\n", error);
        return false;
    }

    return true;
}

bool texture::load_bmp_icon(SDL_Window* window, const std::string_view filepath) noexcept
{
    if (SDL_Surface* bmp_surface = SDL_LoadBMP(filepath.data()))
    {
        SDL_SetWindowIcon(window, bmp_surface);
        SDL_DestroySurface(bmp_surface);

        return true;
    }

    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load window icon from %s: %s",
                 filepath.data(), SDL_GetError());
    return false;
}
