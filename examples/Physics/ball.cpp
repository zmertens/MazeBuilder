#include "ball.h"

#include <box2d/box2d.h>

#include <SDL3/SDL.h>

#include "mouse_states.h"
#include "texture.h"

#include <MazeBuilder/randomizer.h>

ball::ball(float x, float y, float radius, std::unique_ptr<texture> t)
    : m_pos_x{x}, m_pos_y{y}, m_radius{radius}
    , m_texture{std::move(t)}
{
}

void ball::draw(SDL_Renderer* renderer, float pixelsPerMeter, float offsetX, float offsetY) const noexcept
{
    if (B2_IS_NULL(m_bodyId) || !renderer)
    {
        return;
    }

    // Get ball position from Box2D
    b2Vec2 position = b2Body_GetPosition(m_bodyId);

    // Apply proper coordinate transformation (physics to screen)
    float screenX = position.x * pixelsPerMeter + offsetX;
    float screenY = position.y * pixelsPerMeter + offsetY;
    float screenRadius = m_radius * pixelsPerMeter;

    // Draw filled circle using simple pixel filling
    SDL_SetRenderDrawColor(renderer, 0, 100, 255, 255); // Blue
    int iRadius = static_cast<int>(screenRadius);
    for (int y = -iRadius; y <= iRadius; ++y)
    {
        for (int x = -iRadius; x <= iRadius; ++x)
        {
            if (x * x + y * y <= iRadius * iRadius)
            {
                SDL_RenderPoint(renderer, screenX + x, screenY + y);
            }
        }
    }
}

/// @brief Handle mouse/touch dragging of physics balls
/// @param elapsed Delta time in seconds
/// @param mice Current mouse states (in physics coordinates)
/// @param rng Randomizer for any randomness needed
/// @details Allows player to drag balls around the screen by:
///          - Detecting which ball is under the cursor
///          - Applying forces to move ball toward mouse position
///          - Setting velocity directly for responsive control
///          This creates a "mouse joint" effect without actual joints.
void ball::update(float elapsed, const mouse_states& mice, mazes::randomizer& rng) noexcept
{
    if (B2_IS_NULL(m_bodyId))
    {
        return;
    }

    const bool isMouseDown = (mice.leftButton == mouse_states::button_state::DOWN ||
                              mice.leftButton == mouse_states::button_state::PRESSED);

    // Get ball position in physics space
    b2Vec2 ballPos = b2Body_GetPosition(m_bodyId);

    // Mouse position is in physics coordinates
    const float mousePhysX = static_cast<float>(mice.x);
    const float mousePhysY = static_cast<float>(mice.y);

    if (isMouseDown)
    {
        if (!m_isDragging)
        {
            // Check if mouse is over this ball to start dragging
            const float dx = mousePhysX - ballPos.x;
            const float dy = mousePhysY - ballPos.y;
            const float distance = SDL_sqrtf(dx * dx + dy * dy);

            // Check if mouse is over this ball (generous hit detection)
            if (distance <= m_radius * 2.0f)
            {
                m_isDragging = true;

                // Wake up the body
                b2Body_SetAwake(m_bodyId, true);

                // Apply small upward impulse to "pick up" the ball
                b2Vec2 impulse = {0.0f, -0.5f};
                b2Body_ApplyLinearImpulseToCenter(m_bodyId, impulse, true);

                SDL_Log("Ball %d selected for dragging at physics pos (%.2f, %.2f)",
                        m_bodyId.index1, ballPos.x, ballPos.y);
            }
        }

        if (m_isDragging)
        {
            // Continue dragging - apply forces toward mouse
            const b2Vec2 toTarget = {mousePhysX - ballPos.x, mousePhysY - ballPos.y};

            // Apply strong force for responsive dragging
            constexpr float forceScale = 220.0f;
            const b2Vec2 force = {toTarget.x * forceScale, toTarget.y * forceScale};
            b2Body_ApplyForceToCenter(m_bodyId, force, true);

            // Set target velocity for more direct control
            constexpr float speedFactor = 15.0f;
            b2Vec2 targetVelocity = {toTarget.x * speedFactor, toTarget.y * speedFactor};

            // Limit max velocity for stability
            constexpr float maxSpeed = 25.0f;
            const float currentSpeed = b2Length(targetVelocity);
            if (currentSpeed > maxSpeed)
            {
                targetVelocity.x = targetVelocity.x * (maxSpeed / currentSpeed);
                targetVelocity.y = targetVelocity.y * (maxSpeed / currentSpeed);
            }

            b2Body_SetLinearVelocity(m_bodyId, targetVelocity);
        }
    }
    else
    {
        // Mouse released
        if (m_isDragging)
        {
            // Apply release velocity (keep some momentum)
            b2Vec2 currentVel = b2Body_GetLinearVelocity(m_bodyId);
            const b2Vec2 releaseVel = {currentVel.x * 0.8f, currentVel.y * 0.8f};
            b2Body_SetLinearVelocity(m_bodyId, releaseVel);

            SDL_Log("Released ball %d", m_bodyId.index1);
            m_isDragging = false;
        }
    }
}
