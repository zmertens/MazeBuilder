#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <vector>

#include <glm/glm.hpp>

class SdlManager;
class Camera;

class Player final
{
public:
    Player(Camera& camera);

    glm::vec3 getPosition() const;
    void setPosition(const glm::vec3& position);
    void move(const glm::vec3& vel, float dt);
    void input(const SdlManager& sdlManager, const float mouseWheelDelta);
    void update(const float dt, const double timeSinceInit);
    void update(const float dt, const double timeSinceInit,
        const std::vector<glm::vec3>& emptySpaces,
        const glm::vec3& spaceScalar);
    void render() const;
    Camera& getCamera() const;

private:
    static const float scMovementScalar;
    static const float scMouseSensitivity;
    static const glm::vec2 scPlayerSize;
    Camera& mFirstPersonCamera;
    glm::vec3 mStartPosition;
    glm::vec3 mMovementDir;
    bool mMouseLocked;
private:
    glm::vec3 iterateThruSpace(const std::vector<glm::vec3>& emptySpaces,
        const glm::vec3& spaceScalar, const glm::vec3& origin,
        const glm::vec3& dir) const;
    glm::vec3 rectangularCollision(const glm::vec3& origin,
        const glm::vec3& dir, const glm::vec3& objSize,
        const glm::vec3& rectangle,
        const glm::vec3& scalar) const;
};

#endif // PLAYER_HPP
