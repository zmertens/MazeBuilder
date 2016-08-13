#include "Camera.hpp"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

const float Camera::scMaxYawValue = 119.0f;
const float Camera::scMaxPitchValue = 89.0f;
const float Camera::scMaxFieldOfView = 89.0f;
float Camera::sSensitivity = 0.05f;

///**
// * @brief Camera::Camera
// * @param position = glm::vec3(0)
// * @param target = glm::vec3(0, 0, -1)
// * @param up = glm::vec3(0, 1, 0)
// * @param fovy = 65.0f
// * @param near = 0.1f
// * @param far = 100.0f
// */
//Camera::Camera(const glm::vec3& position,
//    const glm::vec3& target,
//    const glm::vec3& up,
//    float fovy, float near, float far)
//: mPosition(position)
//, mTarget(target) // not really used
//, mUp(up) // not really used
//, mYaw(-90.0f)
//, mPitch(0.0f)
//, mFieldOfView(fovy)
//, mNear(near)
//, mFar(far)
//{
//    updateVectors();
//}

/**
 * @brief Camera::Camera
 * @param position = glm::vec3(0)
 * @param yaw = -90.0f
 * @param pitch = 0.0f
 * @param fovy = 65.0f
 * @param near = 0.1f
 * @param far = 100.0f
 */
Camera::Camera(const glm::vec3& position,
    const float yaw, const float pitch,
    float fovy, float near, float far)
: mPosition(position)
, mYaw(yaw)
, mPitch(pitch)
, mFieldOfView(fovy)
, mNear(near)
, mFar(far)
{
    updateVectors();
}

/**
 * (formula) ds = dv * t
 * @brief Camera::move
 * @param velocity
 * @param dt
 */
void Camera::move(const glm::vec3& velocity, float dt)
{
    mPosition = mPosition + (velocity * dt);
}

/**
 * @brief Camera::rotate
 * @param yaw
 * @param pitch
 * @param holdPitch = true
 * @param holdYaw = true
 */
void Camera::rotate(float yaw, float pitch, bool holdPitch, bool holdYaw)
{
    mYaw += yaw * sSensitivity;
    mPitch += pitch * sSensitivity;
    if (holdPitch)
    {
        if (mPitch > scMaxPitchValue)
        {
            mPitch = scMaxPitchValue;
        }
        if (mPitch < -scMaxPitchValue)
        {
            mPitch = -scMaxPitchValue;
        }
    }

    if (holdYaw)
    {
        if (mYaw > scMaxYawValue)
        {
            mYaw = -scMaxYawValue;
        }
        if (mYaw < -scMaxYawValue)
        {
            mYaw = scMaxYawValue;
        }
    }

    updateVectors();
}

/**
 * @brief Camera::getLookAt
 * @return
 */
glm::mat4 Camera::getLookAt() const
{
    return glm::lookAt(mPosition, mPosition + mTarget, mUp);
}

/**
 * @brief Camera::getPerspective
 * @param aspectRatio
 * @return
 */
glm::mat4 Camera::getPerspective(const float aspectRatio) const
{
    return glm::perspective(glm::radians(mFieldOfView), aspectRatio, mNear, mFar);
}

/**
 * @brief Camera::getInfPerspective
 * @param aspectRatio
 * @return
 */
glm::mat4 Camera::getInfPerspective(const float aspectRatio) const
{
    return glm::infinitePerspective(glm::radians(mFieldOfView), aspectRatio, mNear);
}

/**
 * @brief Camera::updateFieldOfView
 * @param dy
 */
void Camera::updateFieldOfView(float dy)
{
    if (mFieldOfView >= 1.0f && mFieldOfView <= scMaxFieldOfView)
    {
        mFieldOfView -= dy;

        if (mFieldOfView <= 1.0f)
            mFieldOfView = 1.0f;
        else if (mFieldOfView >= scMaxFieldOfView)
            mFieldOfView = scMaxFieldOfView;
    }
}

/**
 * @brief Camera::getPosition
 * @return
 */
glm::vec3 Camera::getPosition() const
{
    return mPosition;
}

/**
 * @brief Camera::setPosition
 * @param position
 */
void Camera::setPosition(const glm::vec3& position)
{
    mPosition = position;
}

/**
 * @brief Camera::getTarget
 * @return
 */
glm::vec3 Camera::getTarget() const
{
    return mTarget;
}

/**
 * @brief Camera::setTarget
 * @param target
 */
void Camera::setTarget(const glm::vec3& target)
{
    mTarget = target;
}

/**
 * @brief Camera::getUp
 * @return
 */
glm::vec3 Camera::getUp() const
{
    return mUp;
}

/**
 * @brief Camera::setUp
 * @param up
 */
void Camera::setUp(const glm::vec3& up)
{
    mUp = up;
}

/**
 * @brief Camera::getRight
 * @return
 */
glm::vec3 Camera::getRight() const
{
    return mRight;
}

/**
 * @brief Camera::setRight
 * @param right
 */
void Camera::setRight(const glm::vec3& right)
{
    mRight = right;
}


/**
 * Update target, right, and up vectors using the
 * Yaw and Pitch values (Euler angles).
 * @brief Camera::updateVectors
 */
void Camera::updateVectors()
{
    static const glm::vec3 yAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 target;
    target.x = std::cos(glm::radians(mYaw)) * std::cos(glm::radians(mPitch));
    target.y = std::sin(glm::radians(mPitch));
    target.z = std::sin(glm::radians(mYaw)) * std::cos(glm::radians(mPitch));
    mTarget = glm::normalize(target);
    mRight = glm::normalize(glm::cross(mTarget, yAxis));
    mUp = glm::normalize(glm::cross(mRight, mTarget));
}
