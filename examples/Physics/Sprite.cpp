#include "Sprite.hpp"

#include "SDLHelper.hpp"
#include "Texture.hpp"

#include <SDL3/SDL.h>

#include <MazeBuilder/singleton_base.h>

Sprite::Sprite(const Texture& texture)
    : m_texture(&texture), m_textureRect()
{
    m_textureRect = { 0, 0, texture.getWidth(), texture.getHeight() };
}

Sprite::Sprite(const Texture& texture, const SDL_Rect& rect)
    : m_texture(&texture), m_textureRect(rect)
{
}

void Sprite::draw(RenderStates states) const noexcept {
    if (m_texture) {

        auto* renderer = mazes::singleton_base<SDLHelper>::instance().get()->renderer;

        SDL_FRect dstRect;
        dstRect.x = states.transform.p.x;
        dstRect.y = states.transform.p.y;
        dstRect.w = static_cast<float>(m_textureRect.w);
        dstRect.h = static_cast<float>(m_textureRect.h);

        SDL_RenderTexture(renderer, m_texture->get(), (SDL_FRect*)&m_textureRect, &dstRect);
    }
}
