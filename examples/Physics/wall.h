#ifndef WALL_H
#define WALL_H

#include <memory>

#include <box2d/id.h>

struct SDL_Renderer;

namespace mazes
{
    class randomizer;
}

class texture;

/// @file wall.h
/// @class wall
/// @brief Data class for a wall with physics properties
class wall
{
public:
    explicit wall(float x, float y, unsigned int counter, std::unique_ptr<texture> t, const b2WorldId& world_id);

    [[nodiscard]] b2BodyId get_body_id() const noexcept;

    void draw(SDL_Renderer* renderer, float pixels_per_meter, float offset_x, float offset_y) const noexcept;

    void update(float elapsed, mazes::randomizer& rng) noexcept;

private:
    std::unique_ptr<texture> m_texture;
    b2BodyId m_body_id;
};

#endif // WALL_H
