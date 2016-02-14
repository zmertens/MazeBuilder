#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <memory>

#include <glm/glm.hpp>

class Camera
{
public:
    typedef std::unique_ptr<Camera> Ptr;
public:
//    explicit Camera(const glm::vec3& position = glm::vec3(0),
//        const glm::vec3& target = glm::vec3(0, 0, -1),
//        const glm::vec3& up = glm::vec3(0, 1, 0),
//        float fovy = 65.0f, float near = 0.1f, float far = 100.0f);

    explicit Camera(const glm::vec3& position = glm::vec3(0),
        const float yaw = -90.0f, const float pitch = 0.0f,
        float fovy = 65.0f, float near = 0.1f, float far = 100.0f);

    void move(const glm::vec3& velocity, float dt);
    void rotate(float yaw, float pitch, bool holdPitch = true, bool holdYaw = true);

    glm::mat4 getLookAt() const;
    glm::mat4 getPerspective(const float aspectRatio) const;
    glm::mat4 getInfPerspective(const float aspectRatio) const;

    void updateFieldOfView(float dy);

    glm::vec3 getPosition() const;
    void setPosition(const glm::vec3& position);
    glm::vec3 getTarget() const;
    void setTarget(const glm::vec3& target);
    glm::vec3 getUp() const;
    void setUp(const glm::vec3& up);
    glm::vec3 getRight() const;
    void setRight(const glm::vec3& right);
private:
    static const float scMaxYawValue;
    static const float scMaxPitchValue;
    static const float scMaxFieldOfView;
    static float sSensitivity;
    glm::vec3 mPosition;
    glm::vec3 mTarget;
    glm::vec3 mUp;
    glm::vec3 mRight;
    float mYaw;
    float mPitch;
    float mFieldOfView;
    float mNear;
    float mFar;
private:
    void updateVectors();
};

#endif // CAMERA_HPP
