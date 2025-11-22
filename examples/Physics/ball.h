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

    explicit ball(float x, float y, float radius, std::unique_ptr<texture> t, const b2WorldId& world_id);

    void draw(SDL_Renderer* renderer, float pixel_per_meter, float offset_x, float offset_y) const noexcept;

    void update(float elapsed, const mouse_states& mice, mazes::randomizer& rng) noexcept;

    // Getters
    [[nodiscard]] b2BodyId get_body_id() const noexcept;
    [[nodiscard]] float get_radius() const noexcept;

private:
    float m_pos_x, m_pos_y, m_radius;
    b2BodyId m_body_id;
    std::unique_ptr<texture> m_texture;
    bool m_is_dragging;
};

#endif // BALL_H
