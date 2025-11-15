#ifndef SPRITE_NODE_HPP
#define SPRITE_NODE_HPP

#include "SceneNode.hpp"
#include "Sprite.hpp"

struct SDL_Rect;
class Texture;

class SpriteNode : public SceneNode
{
public:
    explicit SpriteNode(const Texture& texture);
    explicit SpriteNode(const Texture& texture, const SDL_Rect& textureRect);

private:
    void drawCurrent(SDL_Renderer* renderer, RenderStates states) const noexcept override;

    Sprite mSprite;
};

#endif // SPRITE_NODE_HPP
