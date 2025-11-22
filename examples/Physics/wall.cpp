#include "wall.h"

#include <box2d/box2d.h>

#include <SDL3/SDL.h>

#include "texture.h"

#include <MazeBuilder/randomizer.h>

wall::wall(float x, float y, unsigned int counter, std::unique_ptr<texture> t, const b2WorldId& world_id)
    : m_texture{std::move(t)}, m_body_id{b2_nullBodyId}
{
    // Create static wall body
    b2BodyDef wall_def = b2DefaultBodyDef();
    wall_def.type = b2_staticBody;
    wall_def.position = {x, y};
    wall_def.userData = reinterpret_cast<void*>(1000u + counter);
    m_body_id = b2CreateBody(world_id, &wall_def);

    // Wake up the body
    b2Body_SetAwake(m_body_id, true);
}

b2BodyId wall::get_body_id() const noexcept
{
    return m_body_id;
}

void wall::draw(SDL_Renderer* renderer, float pixels_per_meter, float offset_x, float offset_y) const noexcept
{
    // Draw walls (black)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    if (B2_IS_NULL(m_body_id))
    {
        return;
    }

    const b2Vec2 position = b2Body_GetPosition(m_body_id);
    const b2Rot rotation = b2Body_GetRotation(m_body_id);

    // Get the shape from the body
    int shapeCount = b2Body_GetShapeCount(m_body_id);
    if (shapeCount == 0)
    {
        return;
    }

    // Get first shape (walls only have one)
    b2ShapeId shape_ids[1];
    b2Body_GetShapes(m_body_id, shape_ids, 1);
    const b2ShapeId shape_id = shape_ids[0];

    if (B2_IS_NULL(shape_id))
    {
        return;
    }

    // Get polygon shape
    const b2Polygon polygon = b2Shape_GetPolygon(shape_id);

    // Transform and render
    SDL_FPoint points[4];
    for (int i = 0; i < polygon.count && i < 4; ++i)
    {
        // Rotate vertex
        const b2Vec2 rotated = b2RotateVector(rotation, polygon.vertices[i]);
        // Translate to position
        auto [x, y] = b2Add(position, rotated);
        // Convert to screen: physics_coords * scale + offset
        points[i].x = x * pixels_per_meter + offset_x;
        points[i].y = y * pixels_per_meter + offset_y;
    }

    // Draw filled rectangle
    if (polygon.count == 4)
    {
        const float min_x = std::min({points[0].x, points[1].x, points[2].x, points[3].x});
        const float max_x = std::max({points[0].x, points[1].x, points[2].x, points[3].x});
        const float min_y = std::min({points[0].y, points[1].y, points[2].y, points[3].y});
        const float max_y = std::max({points[0].y, points[1].y, points[2].y, points[3].y});

        const SDL_FRect rect = {min_x, min_y, max_x - min_x, max_y - min_y};
        SDL_RenderFillRect(renderer, &rect);
    }
}

void wall::update(float elapsed, mazes::randomizer& rng) noexcept
{
}

