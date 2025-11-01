#include "Sprite.hpp"

#include "SDLHelper.hpp"
#include "Texture.hpp"

#include <MazeBuilder/singleton_base.h>

Sprite::Sprite(const Texture& texture)
    : mTexture(&texture), mTextureRect{ SDL_Rect{ 0, 0, texture.getWidth(), texture.getHeight() } }{


}

Sprite::Sprite(const Texture& texture, const SDL_Rect& rect)
    : mTexture(&texture), mTextureRect{ rect }{

}

void Sprite::draw(RenderStates states) const noexcept {
    if (mTexture) {

        auto rectangleBounds = SDL_Rect{ 0, 0, mTexture->getWidth(), mTexture->getHeight() };

        // Convert source rect from SDL_Rect to SDL_FRect
        SDL_FRect srcRect;
        srcRect.x = static_cast<float>(rectangleBounds.x);
        srcRect.y = static_cast<float>(rectangleBounds.y);
        srcRect.w = static_cast<float>(rectangleBounds.w);
        srcRect.h = static_cast<float>(rectangleBounds.h);

        SDL_FRect dstRect;
        dstRect.x = states.transform.p.x;
        dstRect.y = states.transform.p.y;
        dstRect.w = static_cast<float>(rectangleBounds.w);
        dstRect.h = static_cast<float>(rectangleBounds.h);

        SDL_RenderTexture(mazes::singleton_base<SDLHelper>::instance().get()->renderer, mTexture->get(), &srcRect, &dstRect);
    }
}

/// @brief 
/// @param texture 
/// @param resetRect false
void Sprite::setTexture(const Texture& texture, bool resetRect) {
    
    mTexture = &texture;

    if (resetRect) {

        mTextureRect = SDL_Rect{ 0, 0, texture.getWidth(), texture.getHeight() };
    }
}
