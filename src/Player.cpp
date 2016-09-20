#include "Player.hpp"

#include <algorithm>
#include <iterator>

#include "engine/SdlWindow.hpp"
#include "engine/Camera.hpp"
#include "engine/Utils.hpp"

#include "Level.hpp"

const float Player::sMouseFactor = 1.0f;
float Player::sMvFactor = sInitMvFactor;

/**
 * @brief Player::Player
 * @param camera
 * @param level
 */
Player::Player(Camera& camera, Level& level)
: mFirstPersonCamera(camera)
, mLevel(level)
, mStartPosition(camera.getPosition())
, mMovementDir(glm::vec3(0))
, mPower(Power::Type::None)
, mPowerUpTimer(0.0)
, mShooting(false)
, mMouseLocked(false)
, mHealth(100.0f)
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
 * @param mosueStates
 * @param coords
 * @param inputs
 */
void Player::input(const SdlWindow& sdlManager, const float mouseWheelDelta,
    const int32_t mouseStates,
    const glm::vec2& coords,
    std::unordered_map<uint8_t, bool> inputs)
{
    if (mouseStates & SDL_BUTTON(SDL_BUTTON_LEFT))
    {
        mShooting = true;
    }
    else
        mShooting = false;

    if (mouseStates & SDL_BUTTON(SDL_BUTTON_RIGHT))
    {
        sMvFactor = 40.0f;
    }
    else
    {
        sMvFactor = sInitMvFactor;
    }

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
            mFirstPersonCamera.rotate(xOffset * sMouseFactor, 0.0f /*yOffset * sMouseFactor*/, false, false);
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
    // if (mHealth < 0.0f)
    //     SDL_Log("Player dead");

    if (mPowerUpTimer > sPowerUpLength)
    {
        mPower = Power::Type::None;
        mPowerUpTimer = 0.0f;
    }

    if (mPower == Power::Type::None)
    {
        if (isOnPoint(getPosition(), mLevel.getSpeedPowerUps()))
            mPower = Power::Type::Speed;
        else if (isOnPoint(getPosition(), mLevel.getStrengthPowerUps()))
            mPower = Power::Type::Strength;
        else if (isOnPoint(getPosition(), mLevel.getInvinciblePowerUps()))
            mPower = Power::Type::Immunity;
    }
    else
        mPowerUpTimer += dt;

    if (glm::length(mMovementDir) > 0)
    {
        if (sCollisions)
        {
            glm::vec3 origin (getPosition());
            // R(t) = P + Vt
            glm::vec3 direction (origin + glm::normalize(mMovementDir * dt));
            glm::vec3 c (Utils::collision(mLevel.getEmptySpace(), mLevel.getTileScalar(), origin, direction, glm::vec3(sPlayerSize)));
            mMovementDir *= c;
            mMovementDir.y = 0.0f;
        }

        if (mPower == Power::Type::Speed)
            mFirstPersonCamera.move(mMovementDir, 1.25 * sMvFactor * dt);
        else 
            mFirstPersonCamera.move(mMovementDir, sMvFactor * dt);
        
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
float Player::getPlayerSize() const
{
    return sPlayerSize;
}

// std::vector<Bullet> Player::getBullets() const
// {
//     return mBullets;
// }

bool Player::isShooting() const
{
    return mShooting;
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
    return sCollisions;
}

Power::Type Player::getPower() const
{
    return mPower;
}

void Player::setPower(Power::Type type)
{
    mPower = type;
}

float Player::getHealth() const
{
    return mHealth;
}

void Player::inflictDamage()
{
    mHealth -= Utils::getRandomFloat(sMinDamage, sMaxDamage);
    if (mPower != Power::Type::Immunity && (sMvFactor - 1.0f > 2.5f))
        sMvFactor -= 1.0f;
}

bool Player::isOnExit() const
{
    return isOnPoint(getPosition(), mLevel.getExitPoints());
}

/**
 * Check if the length of the distance from the player
 * to the point is equal to the distance from the center of the sprite to it's boundary.
 * @brief Player::isOnPoint
 * @param origin
 * @return
 */
bool Player::isOnPoint(const glm::vec3& origin, const std::vector<glm::vec3>& points) const
{
    auto exited = std::find_if(points.begin(), points.end(),
        [&] (const glm::vec3& point)->bool {
            return glm::length(point - origin) < mLevel.getSpriteHalfWidth();
    });

    return (exited != points.end());
}