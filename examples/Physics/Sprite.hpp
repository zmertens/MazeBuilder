#ifndef SPRITE_HPP
#define SPRITE_HPP

#include "RenderStates.hpp"
#include "Transformable.hpp"

#include <SDL3/SDL_rect.h>

struct SDL_Renderer;
class Texture;

/// @brief 
class Sprite : public Transformable
{
public:
    explicit Sprite(const Texture& texture);

    explicit Sprite(const Texture&& texture) = delete;

    explicit Sprite(const Texture& texture, const SDL_Rect& rect);

    void draw(SDL_Renderer* renderer, RenderStates states) const noexcept;

    /// @brief Change the source texture of the sprite
    /// @param texture New texture (must exist as long as sprite uses it)
    /// @param resetRect Should the texture rect be reset to the size of the new texture?
    void setTexture(const Texture& texture, bool resetRect = false);

    /// @brief Disallow setting from a temporary texture
    void setTexture(const Texture&& texture, bool resetRect = false) = delete;

    /// @brief Set the sub-rectangle of the texture that the sprite will display
    /// @param rectangle Rectangle defining the region of the texture to display
    void setTextureRect(const SDL_Rect& rectangle);

private:
    const Texture* mTexture;
    SDL_Rect mTextureRect;
}; // Sprite class

#endif // SPRITE_HPP
