#include "ball.h"

#include <box2d/box2d.h>

#include <SDL3/SDL.h>

#include "mouse_states.h"
#include "texture.h"

#include <MazeBuilder/randomizer.h>

ball::ball(float x, float y, float radius, std::unique_ptr<texture> t, const b2WorldId& world_id)
    : m_pos_x{x}, m_pos_y{y}, m_radius{radius}
    , m_texture{std::move(t)}
    , m_body_id{b2_nullBodyId}
    , m_is_dragging{false}
{

    // Create dynamic body for the ball
    b2BodyDef ball_def = b2DefaultBodyDef();
    ball_def.type = b2_dynamicBody;
    ball_def.position = {x, y};
    ball_def.linearDamping = 0.1f;
    ball_def.angularDamping = 0.1f;

    m_body_id = b2CreateBody(world_id, &ball_def);

    // Create circle shape
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;  // Standard density

    b2Circle circle = {{0.0f, 0.0f}, radius};
    b2CreateCircleShape(m_body_id, &shapeDef, &circle);
}

void ball::draw(SDL_Renderer* renderer, float pixel_per_meter, float offsetX, float offset_y) const noexcept
{
    if (B2_IS_NULL(m_body_id) || !renderer)
    {
        return;
    }

    // Get ball position from Box2D
    b2Vec2 position = b2Body_GetPosition(m_body_id);

    // Apply proper coordinate transformation (physics to screen)
    float screenX = position.x * pixel_per_meter + offsetX;
    float screenY = position.y * pixel_per_meter + offset_y;
    float screenRadius = m_radius * pixel_per_meter;

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
    if (B2_IS_NULL(m_body_id))
    {
        return;
    }

    const bool isMouseDown = (mice.leftButton == mouse_states::button_state::DOWN ||
                              mice.leftButton == mouse_states::button_state::PRESSED);

    // Get ball position in physics space
    b2Vec2 ballPos = b2Body_GetPosition(m_body_id);

    // Mouse position is in physics coordinates
    const auto mousePhysX = static_cast<float>(mice.x);
    const auto mousePhysY = static_cast<float>(mice.y);

    if (isMouseDown)
    {
        if (!m_is_dragging)
        {
            // Check if mouse is over this ball to start dragging
            const float dx = mousePhysX - ballPos.x;
            const float dy = mousePhysY - ballPos.y;
            const float distance = SDL_sqrtf(dx * dx + dy * dy);

            // Check if mouse is over this ball (generous hit detection)
            if (distance <= m_radius * 2.0f)
            {
                m_is_dragging = true;

                // Wake up the body
                b2Body_SetAwake(m_body_id, true);

                // Apply small upward impulse to "pick up" the ball
                b2Vec2 impulse = {0.0f, -0.5f};
                b2Body_ApplyLinearImpulseToCenter(m_body_id, impulse, true);

                SDL_Log("Ball %d selected for dragging at physics pos (%.2f, %.2f)",
                        m_body_id.index1, ballPos.x, ballPos.y);
            }
        }

        if (m_is_dragging)
        {
            // Continue dragging - apply forces toward mouse
            const b2Vec2 toTarget = {mousePhysX - ballPos.x, mousePhysY - ballPos.y};

            // Apply strong force for responsive dragging
            constexpr float forceScale = 220.0f;
            const b2Vec2 force = {toTarget.x * forceScale, toTarget.y * forceScale};
            b2Body_ApplyForceToCenter(m_body_id, force, true);

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

            b2Body_SetLinearVelocity(m_body_id, targetVelocity);
        }
    }
    else
    {
        // Mouse released
        if (m_is_dragging)
        {
            // Apply release velocity (keep some momentum)
            b2Vec2 currentVel = b2Body_GetLinearVelocity(m_body_id);
            const b2Vec2 releaseVel = {currentVel.x * 0.8f, currentVel.y * 0.8f};
            b2Body_SetLinearVelocity(m_body_id, releaseVel);

            SDL_Log("Released ball %d", m_body_id.index1);
            m_is_dragging = false;
        }
    }
}

b2BodyId ball::get_body_id() const noexcept
{
    return m_body_id;
}

float ball::get_radius() const noexcept
{
    return m_radius;
}
