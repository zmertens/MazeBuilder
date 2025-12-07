#include "texture.h"

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <SDL3/SDL.h>

#include <MazeBuilder/enums.h>

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
bool texture::load_from_file(std::string_view filepath, std::uint32_t channel_offset) noexcept
{
    this->free();

    // CRITICAL: Set flip BEFORE loading the image
    stbi_set_flip_vertically_on_load(true);

    int width, height;
    int n; // n stores number of components (channels)

    // Force RGBA (4 components) for consistency
    auto *data = stbi_load(filepath.data(), &width, &height, &n, 4);

    if (data == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "stbi_load %s failed: %s\n",
                     filepath.data(), stbi_failure_reason());
        return false;
    }

    SDL_Log("Loaded texture %s: %dx%d, %d channels (forced to RGBA)",
            filepath.data(), width, height, n);

    glGenTextures(1, &m_texture);
    glActiveTexture(GL_TEXTURE0 + channel_offset);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Upload texture data - now we know it's always RGBA
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data);

    // Generate mipmaps for better quality
    glGenerateMipmap(GL_TEXTURE_2D);

    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "OpenGL error after loading %s: 0x%x\n",
                     filepath.data(), error);
        stbi_image_free(data);
        return false;
    }

    m_width = width;
    m_height = height;

    stbi_image_free(data);

    SDL_Log("Successfully created OpenGL texture %u for %s", m_texture, filepath.data());
    return true;
}

bool texture::load_target(int w, int h) noexcept
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "OpenGL error creating render target %dx%d: 0x%x\n", w, h, error);
        return false;
    }

    SDL_Log("Created render target texture %u: %dx%d", m_texture, w, h);
    return true;
}

bool texture::load_from_str(std::string_view str, int cellSize) noexcept
{
    // This method appears to be for creating textures from string data
    // Implementation would depend on specific use case
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "load_from_str not yet implemented");
    return false;
}

bool texture::load_bmp_icon(SDL_Window *window, std::string_view filepath) noexcept
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
