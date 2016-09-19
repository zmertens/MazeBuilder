#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <vector>
#include <unordered_map>
#include <cstdint>

#include <glm/glm.hpp>

#include "PowerUp.hpp"

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

    float getPlayerSize() const;

    bool isShooting() const;

    bool getMouseLocked() const;
    void setMouseLocked(bool mouseLocked);

    bool getCollisions() const;

    Power::Type getPower() const;
    void setPower(Power::Type type);

    float getHealth() const;
    void inflictDamage();

    bool isOnExit() const;

private:
    static const float sMouseFactor;
    static constexpr float sInitMvFactor = 25.0f;
    static constexpr float sPowerUpLength = 20.0f;
    static float sMvFactor;
    static constexpr float sPlayerSize = 0.2f;
    static constexpr bool sCollisions = false;
    static constexpr float sMinDamage = 0.3f;
    static constexpr float sMaxDamage = 1.0f;
    
    Camera& mFirstPersonCamera;
    Level& mLevel;
    glm::vec3 mStartPosition;
    glm::vec3 mMovementDir;
    
    Power::Type mPower;
    float mPowerUpTimer;
    bool mShooting;
    bool mMouseLocked;
    float mHealth;
private:
    bool isOnPoint(const glm::vec3& origin, const std::vector<glm::vec3>& points) const;
};

#endif // PLAYER_HPP
