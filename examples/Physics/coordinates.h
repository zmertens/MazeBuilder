#ifndef COORDINATES_H
#define COORDINATES_H

#include <box2d/math_functions.h>
#include <SDL3/SDL_rect.h>

/// @brief Convert screen coordinates to Box2D physics world coordinates
/// @param screen_x X coordinate in screen/pixel space
/// @param screen_y Y coordinate in screen/pixel space
/// @param offset_x
/// @param offset_y
/// @param pixels_per_meter
/// @return b2Vec2 position in physics world space (meters)
/// @details This accounts for the maze offset and pixel-to-meter conversion.
///          Essential for mouse interaction with physics objects.
static b2Vec2 screen_to_physics_coords(const float screen_x, const float screen_y, const float offset_x, const float offset_y, const float pixels_per_meter) noexcept
{
    // Convert from screen coordinates to physics coordinates
    // by accounting for offset and scale
    const float phys_x = (screen_x - offset_x) / pixels_per_meter;
    const float phys_y = (screen_y - offset_y) / pixels_per_meter;

    return {phys_x, phys_y};
}

/// @brief Convert Box2D physics coordinates to screen coordinates
/// @param phys_x X position in physics world (meters)
/// @param phys_y Y position in physics world (meters)
/// @param offset_x
/// @param offset_y
/// @param pixels_per_meter
/// @return SDL_FPoint position in screen/pixel space
/// @details Used for rendering physics objects at correct screen positions
static SDL_FPoint physics_to_screen(const float phys_x, const float phys_y, const float offset_x, const float offset_y, const float pixels_per_meter) noexcept
{
    return {phys_x * pixels_per_meter + offset_x, phys_y * pixels_per_meter + offset_y};
}

#endif // COORDINATES_H
