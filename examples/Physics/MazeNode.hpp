#ifndef MAZE_NODE_HPP
#define MAZE_NODE_HPP

#include "SceneNode.hpp"
#include "Sprite.hpp"

class Texture;
class MazeLayout;

class MazeNode : public SceneNode
{
public:
    explicit MazeNode(const Texture& texture);

private:
    void drawCurrent(SDL_Renderer* renderer, RenderStates states) const noexcept override;

    Sprite mSprite;
};

#endif // MAZE_NODE_HPP

