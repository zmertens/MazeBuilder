#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <vector>
#include <unordered_map>
#include <cstdint>

#include <glm/glm.hpp>

#include "Bullet.hpp"

class SdlWindow;
class Camera;
class Level;

class Player final
{
public:
    Player(Camera& camera, Level& level);

    glm::vec3 getPosition() const;
    void setPosition(const glm::vec3& position);
    void move(const glm::vec3& vel, float dt);
    void input(const SdlWindow& sdlManager, const float mouseWheelDelta,
        const int32_t mouseStates,
        const glm::vec2& coords,
        std::unordered_map<uint8_t, bool> inputs);
    void update(const float dt, const double timeSinceInit);
    void render() const;
    Camera& getCamera() const;

    glm::vec2 getPlayerSize() const;

    // std::vector<Bullet> getBullets() const;
    bool isShooting() const;

    bool getMouseLocked() const;
    void setMouseLocked(bool mouseLocked);

    bool getCollisions() const;
    void setCollisions(bool collisions);

    bool getInvincible() const;
    void setInvincible(bool invincible);

    bool getSpeed() const;
    void setSpeed(bool speed);

    bool getInfAmmo() const;
    void setInfAmmo(bool infAmmo);

    bool getStrength() const;
    void setStrength(bool strength);

    void inflictDamage(const float min, const float max);

private:
    static const float scMouseSensitivity;
    static constexpr float scOgMovementScalar = 25.0f;
    static constexpr std::size_t scMaxBullets = 25;
    static float scMovementScalar;
    const glm::vec2 cPlayerSize;
    Camera& mFirstPersonCamera;
    Level& mLevel;
    glm::vec3 mStartPosition;
    glm::vec3 mMovementDir;
    // std::vector<Bullet> mBullets;
    bool mShooting;
    bool mMouseLocked;
    float mHealth;
    bool mCollisions;
    bool mInvincible;
    bool mSpeed;
    bool mInfAmmo;
    bool mStrength;
private:
    glm::vec3 collision(const std::vector<glm::vec3>& emptySpaces,
        const glm::vec3& spaceScalar, const glm::vec3& origin,
        const glm::vec3& dir) const;
    glm::vec3 rectangularCollision(const glm::vec3& origin,
        const glm::vec3& dir, const glm::vec3& objSize,
        const glm::vec3& rectangle,
        const glm::vec3& scalar) const;
    bool isOnExitPoint(const glm::vec3& origin) const;
    bool isOnSpeedPowerUp(const glm::vec3& origin) const;
    bool isOnStrengthPowerUp(const glm::vec3& origin) const;
    bool isOnInvinciblePowerUp(const glm::vec3& origin) const;
};

#endif // PLAYER_HPP
