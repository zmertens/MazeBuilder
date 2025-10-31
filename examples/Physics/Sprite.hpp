#ifndef SPRITE_HPP
#define SPRITE_HPP

#include <SDL3/SDL.h>

#include "Drawable.hpp"
#include "RenderStates.hpp"
#include "Transformable.hpp"

class Texture;

/// @brief 
class Sprite : public Drawable, Transformable {
public:
    explicit Sprite(const Texture& texture);
    Sprite(const Texture& texture, const SDL_Rect& rect);

    void draw(RenderStates states) const noexcept;

private:
    const Texture* m_texture;
    SDL_Rect m_textureRect;
}; // Sprite class

#endif // SPRITE_HPP
