#include "Player.hpp"

#include <algorithm>

#include "engine/SdlWindow.hpp"
#include "engine/Camera.hpp"

#include "Level.hpp"

const float Player::scMouseSensitivity = 1.0f;
float Player::scMovementScalar = 40.0f;

/**
 * @brief Player::Player
 * @param camera
 * @param level
 */
Player::Player(Camera& camera, Level& level)
: cPlayerSize(0.2f)
, mFirstPersonCamera(camera)
, mLevel(level)
, mStartPosition(camera.getPosition())
, mMovementDir(glm::vec3(0))
, mMouseLocked(false)
, mCollisions(true)
, mInvincible(false)
, mSpeed(false)
, mInfAmmo(false)
, mStrength(false)
{

}

/**
 * @brief Player::getPosition
 * @return
 */
glm::vec3 Player::getPosition() const
{
    return mFirstPersonCamera.getPosition();
}

/**
 * @brief Player::setPosition
 * @param position
 */
void Player::setPosition(const glm::vec3& position)
{
    mFirstPersonCamera.setPosition(position);
}

/**
 * @brief Player::move
 * @param vel
 * @param dt
 */
void Player::move(const glm::vec3& vel, float dt)
{
    mFirstPersonCamera.move(vel, dt);
}

/**
 * (currentMouseStates & SDL_BUTTON(SDL_BUTTON_LEFT)
 * @brief Player::input
 * @param sdlManager
 * @param mouseWheelDelta
 * @param coords
 * @param inputs
 */
void Player::input(const SdlWindow& sdlManager, const float mouseWheelDelta,
    const glm::vec2& coords,
    std::unordered_map<uint8_t, bool> inputs)
{
    const float winCenterX = static_cast<float>(sdlManager.getWindowWidth()) * 0.5f;
    const float winCenterY = static_cast<float>(sdlManager.getWindowHeight()) * 0.5f;

    // keyboard movements
    if (inputs[SDL_SCANCODE_W])
    {
        inputs.at(SDL_SCANCODE_W) = false;
        mMovementDir += mFirstPersonCamera.getTarget();
    }

    if (inputs[SDL_SCANCODE_S])
    {
        inputs.at(SDL_SCANCODE_S) = false;
        mMovementDir -= mFirstPersonCamera.getTarget();
    }

    if (inputs[SDL_SCANCODE_A])
    {
        inputs.at(SDL_SCANCODE_A) = false;
        mMovementDir -= mFirstPersonCamera.getRight();
    }

    if (inputs[SDL_SCANCODE_D])
    {
        inputs.at(SDL_SCANCODE_D) = false;
        mMovementDir += mFirstPersonCamera.getRight();
    }

    // mouse wheel events
    if (mouseWheelDelta != 0)
        mFirstPersonCamera.updateFieldOfView(mouseWheelDelta);

    // rotations (mouse movements)
    if (mMouseLocked)
    {
        const float xOffset = coords.x - winCenterX;
        const float yOffset = winCenterY - coords.y;

        if (xOffset || yOffset)
        {
            mFirstPersonCamera.rotate(xOffset * scMouseSensitivity, 0.0f /*yOffset * scMouseSensitivity*/, false, false);
            SDL_WarpMouseInWindow(sdlManager.getSdlWindow(), winCenterX, winCenterY);
        }
    }
}

/**
 * @brief Player::update
 * @param dt
 * @param timeSinceInit
 */
void Player::update(const float dt, const double timeSinceInit)
{
    if (glm::length(mMovementDir) > 0)
    {
        if (mCollisions)
        {
            glm::vec3 origin (getPosition());
            // R(t) = P + Vt
            glm::vec3 direction (origin + glm::normalize(mMovementDir * scMovementScalar * dt));
            glm::vec3 c (collision(mLevel.getEmptySpace(), mLevel.getTileScalar(), origin, direction));
            mMovementDir *= c;
            mMovementDir.y = 0.0f;
        }

        mFirstPersonCamera.move(mMovementDir, scMovementScalar * dt);

        if (isOnExitPoint(getPosition()))
        {

        }

        // reset movement direction every iteration
        mMovementDir = glm::vec3(0);
    }
    // else no movement
}

/**
 * In first person, the player's hands are rendered.
 * In third person, the entire player is rendered.
 * @brief Player::render
 */
void Player::render() const
{

}

/**
 * @brief Player::getCamera
 * @return
 */
Camera& Player::getCamera() const
{
    return mFirstPersonCamera;
}

/**
 * @brief Player::getPlayerSize
 * @return
 */
glm::vec2 Player::getPlayerSize() const
{
    return cPlayerSize;
}

/**
 * @brief Player::getMouseLocked
 * @return
 */
bool Player::getMouseLocked() const
{
    return mMouseLocked;
}

/**
 * @brief Player::setMouseLocked
 * @param mouseLocked
 */
void Player::setMouseLocked(bool mouseLocked)
{
    mMouseLocked = mouseLocked;
}

bool Player::getCollisions() const
{
    return mCollisions;
}

void Player::setCollisions(bool collisions)
{
    mCollisions = collisions;
}

bool Player::getInvincible() const
{
    return mInvincible;
}

void Player::setInvincible(bool invincible)
{
    mInvincible = invincible;
}

bool Player::getSpeed() const
{
    return mSpeed;
}

void Player::setSpeed(bool speed)
{
    mSpeed = speed;
}

bool Player::getInfAmmo() const
{
    return mInfAmmo;
}

void Player::setInfAmmo(bool infAmmo)
{
    mInfAmmo = infAmmo;
}

bool Player::getStrength() const
{
    return mStrength;
}

void Player::setStrength(bool strength)
{
    mStrength = strength;
}

/**
 * @brief Player::collision
 * @param emptySpaces
 * @param spaceScalar
 * @param origin
 * @param dir
 * @return
 */
glm::vec3 Player::collision(const std::vector<glm::vec3>& emptySpaces,
    const glm::vec3& spaceScalar,
    const glm::vec3& origin,
    const glm::vec3& dir) const
{
    glm::vec3 collisionVec (1);
    for (auto& emptiness : emptySpaces)
    {
        collisionVec *= rectangularCollision(origin, dir,
            glm::vec3(cPlayerSize.x, 0, cPlayerSize.y), emptiness, spaceScalar);
    }

    return collisionVec;
}

/**
 * Returns zero vector if no movement whatsoever,
 * else it returns 1 along the axis of movement.
 * @brief Player::rectangularCollision
 * @param origin
 * @param dir
 * @param objSize
 * @param rectangle
 * @param scalar
 * @return
 */
glm::vec3 Player::rectangularCollision(const glm::vec3& origin,
    const glm::vec3& dir, const glm::vec3& objSize,
    const glm::vec3& rectangle,
    const glm::vec3& scalar) const
{
    glm::vec3 result (0.0f, 1.0f, 0.0f);

    if (dir.x + objSize.x < rectangle.x * scalar.x ||
       dir.x - objSize.x > (rectangle.x + 1.0f) * scalar.x  ||
       origin.z + objSize.z < rectangle.z * scalar.z ||
       origin.z - objSize.z > (rectangle.z + 1.0f) * scalar.z)
    {
        result.x = 1.0f;
    }

    if (origin.x + objSize.x < rectangle.x * scalar.x  ||
       origin.x - objSize.x > (rectangle.x + 1.0f) * scalar.x  ||
       dir.z + objSize.z < rectangle.z * scalar.z ||
       dir.z - objSize.z > (rectangle.z + 1.0f) * scalar.z)
    {
        result.z = 1.0f;
    }

    return result;
}

/**
 * Check if the length of the distance from the player
 * to the point is equal to half the size of the sprite.
 * @brief Player::isOnExitPoint
 * @param origin
 * @return
 */
bool Player::isOnExitPoint(const glm::vec3& origin) const
{
    const auto& exitPoints = mLevel.getExitPoints();

    auto exited = std::find_if(exitPoints.begin(), exitPoints.end(),
        [&] (const glm::vec3& point)->bool {
            return glm::length(point - origin) < mLevel.getSpriteHalfWidth();
    });

    return (exited != exitPoints.end());
}

/**
 * @brief Player::isOnSpeedPowerUp
 * @param origin
 * @return
 */
bool Player::isOnSpeedPowerUp(const glm::vec3& origin) const
{
    return false;
}

/**
 * @brief Player::isOnRechargePowerUp
 * @param origin
 * @return
 */
bool Player::isOnRechargePowerUp(const glm::vec3& origin) const
{
    return false;
}

/**
 * @brief Player::isOnInvinciblePowerUp
 * @param origin
 * @return
 */
bool Player::isOnInvinciblePowerUp(const glm::vec3& origin) const
{
    return false;
}
