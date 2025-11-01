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

        // Convert source rect from SDL_Rect to SDL_FRect
        SDL_FRect srcRect;
        srcRect.x = static_cast<float>(m_textureRect.x);
        srcRect.y = static_cast<float>(m_textureRect.y);
        srcRect.w = static_cast<float>(m_textureRect.w);
        srcRect.h = static_cast<float>(m_textureRect.h);

        SDL_FRect dstRect;
        dstRect.x = states.transform.p.x;
        dstRect.y = states.transform.p.y;
        dstRect.w = static_cast<float>(m_textureRect.w);
        dstRect.h = static_cast<float>(m_textureRect.h);

        SDL_RenderTexture(renderer, m_texture->get(), &srcRect, &dstRect);
    }
}
