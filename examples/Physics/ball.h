#ifndef BALL_H
#define BALL_H

#include <memory>

#include <box2d/id.h>

struct SDL_Renderer;
namespace mazes
{
    class randomizer;
}

struct mouse_states;
class texture;

/// @file ball.h
/// @class ball
/// @brief Data class for a ball with physics properties
class ball
{
public:

    explicit ball(float x, float y, float radius, std::unique_ptr<texture> t);

    void draw(SDL_Renderer* renderer, float pixelsPerMeter, float offsetX, float offsetY) const noexcept;

    void update(float elapsed, const mouse_states& mice, mazes::randomizer& rng) noexcept;

    // Setters
    void setBodyId(b2BodyId bodyId) noexcept { m_bodyId = bodyId; }

    // Getters
    [[nodiscard]] b2BodyId getBodyId() const noexcept { return m_bodyId; }
    [[nodiscard]] float getRadius() const noexcept { return m_radius; }

private:
    float m_pos_x, m_pos_y, m_radius;
    b2BodyId m_bodyId = b2_nullBodyId;  // Initialize to null!
    std::unique_ptr<texture> m_texture;
    bool m_isDragging = false;
};

#endif // BALL_H
