#include "texture.h"

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

texture::texture(texture &&other) noexcept
{
    m_texture = other.m_texture;
    m_width = other.m_width;
    m_height = other.m_height;

    other.m_texture = 0;
    other.m_width = 0;
    other.m_height = 0;
}

texture &texture::operator=(texture &&other) noexcept
{
    if (this != &other)
    {
        free();

        m_texture = other.m_texture;
        m_width = other.m_width;
        m_height = other.m_height;

        other.m_texture = 0;
        other.m_width = 0;
        other.m_height = 0;
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

// Load an image file using stb_image and create an SDL texture
bool texture::load_from_file(const std::string_view filepath, const std::uint32_t channel_offset) noexcept
{
    this->free();

    stbi_set_flip_vertically_on_load(true);

    int width, height;
    // n stores number of components (channels)
    int n;

    // Force RGBA (4 components) for consistency
    auto *data = stbi_load(filepath.data(), &width, &height, &n, 4);

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
/// @param data
/// @param width
/// @param height
/// @param channel_offset 0
bool texture::load_from_memory(const std::uint8_t* data, const int width, const int height,
    const std::uint32_t channel_offset) noexcept
{
    if (data == nullptr || width <= 0 || height <= 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Invalid parameters for load_from_memory\n");
        return false;
    }

    this->free();

    glGenTextures(1, &m_texture);
    glActiveTexture(GL_TEXTURE0 + channel_offset);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload texture data - RGBA format
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
        GL_RGBA,GL_UNSIGNED_BYTE, data);

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

bool texture::load_bmp_icon(SDL_Window *window, const std::string_view filepath) noexcept
{
    if (SDL_Surface *bmp_surface = SDL_LoadBMP(filepath.data()))
    {
        SDL_SetWindowIcon(window, bmp_surface);
        SDL_DestroySurface(bmp_surface);

        return true;
    }

    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to load window icon from %s: %s",
                 filepath.data(), SDL_GetError());
    return false;
}
