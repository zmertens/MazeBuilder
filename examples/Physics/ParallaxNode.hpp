#ifndef PARALLAX_NODE_HPP
#define PARALLAX_NODE_HPP

#include "SceneNode.hpp"
#include "Sprite.hpp"

#include <SDL3/SDL.h>

class Texture;

/// @brief A scene node that renders a parallax scrolling background
/// @details Draws the texture twice to create seamless scrolling effect
class ParallaxNode : public SceneNode
{
public:
    explicit ParallaxNode(const Texture& texture, float scrollSpeed = 1.0f);

    /// @brief Set the scrolling speed (negative for left scroll, positive for right)
    void setScrollSpeed(float speed) noexcept { mScrollSpeed = speed; }

    /// @brief Get the current scrolling speed
    [[nodiscard]] float getScrollSpeed() const noexcept { return mScrollSpeed; }

    /// @brief Set the vertical offset for positioning the parallax layer
    void setVerticalOffset(float offset) noexcept { mVerticalOffset = offset; }

    /// @brief Set the scale factor for the texture
    void setScale(float scale) noexcept { mScale = scale; }

private:
    void updateCurrent(float dt, CommandQueue& commands) noexcept override;
    void drawCurrent(SDL_Renderer* renderer, RenderStates states) const noexcept override;

    const Texture& mTexture;
    float mScrollOffset;
    float mScrollSpeed;
    float mVerticalOffset;
    float mScale;
};

#endif // PARALLAX_NODE_HPP

