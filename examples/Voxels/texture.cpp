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

// Load an image file using stb_image and create an SDL texture
bool texture::load_from_file(std::string_view filepath, std::uint32_t channel_offset) noexcept
{
    this->free();

    glGenTextures(1, &m_texture);
    glActiveTexture(GL_TEXTURE0 + channel_offset);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height;
    // n stores bits-per-pixel
    int n;
    auto *data = stbi_load(filepath.data(), &width, &height, &n, 0);
    // ... replace '0' with '1'..'4' to force that many components per pixel
    if (data == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "std_load %s failed!!\n", filepath.data());

        return false;
    }
    stbi_set_flip_vertically_on_load(true);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    return true;
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
