#include "Transform.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * @brief Transform::Transform
 */
Transform::Transform()
: mTranslation(glm::vec3(0.0f))
, mRotation(glm::vec3(0.0f))
, mScale(glm::vec3(1.0f))
{

}

/**
 * @brief Transform::Transform
 * @param translation
 * @param rotation
 * @param scale
 */
Transform::Transform(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
: mTranslation(translation)
, mRotation(rotation)
, mScale(scale)
{

}

/**
 * @brief Transform::getModel
 * @return
 */
glm::mat4 Transform::getModel() const
{
    glm::mat4 transMat = glm::translate(mTranslation);
    glm::mat4 scaleMat = glm::scale(mScale);
    glm::mat4 rotX = glm::rotate(glm::radians(mRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotY = glm::rotate(glm::radians(mRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotZ = glm::rotate(glm::radians(mRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    return transMat * (rotX * rotY * rotZ) * scaleMat;
}

/**
 * @ISSUE -- Not thoroughly tested
 * @brief Transform::getModelCentered
 * @return
 */
glm::mat4 Transform::getModelCentered() const
{
    glm::mat4 transMat = glm::translate(mTranslation);
    glm::mat4 transUnCenter = glm::translate(glm::vec3(-0.5f * mScale.x, -0.5f * mScale.y, -0.5f * mScale.z));
    glm::mat4 transCenter = glm::translate(glm::vec3(0.5f * mScale.x, 0.5f * mScale.y, 0.5f * mScale.z));
    glm::mat4 scaleMat = glm::scale(mScale);
    glm::mat4 rotX = glm::rotate(glm::radians(mRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotY = glm::rotate(glm::radians(mRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotZ = glm::rotate(glm::radians(mRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    return transMat * transCenter * (rotX * rotY * rotZ) * transUnCenter * scaleMat;
}

/**
 * @brief Transform::getModelView
 * @param view
 * @return
 */
glm::mat4 Transform::getModelView(const glm::mat4& view) const
{
    return view * getModel();
}

/**
 * @brief Transform::getMVP
 * @param view
 * @param projection
 * @return
 */
glm::mat4 Transform::getMVP(const glm::mat4& view, const glm::mat4& projection) const
{
    return projection * view * getModel();
}

/**
 * @brief Transform::getTranslation
 * @return
 */
glm::vec3 Transform::getTranslation() const
{
    return mTranslation;
}

/**
 * @brief Transform::setTranslation
 * @param translation
 */
void Transform::setTranslation(const glm::vec3& translation)
{
    mTranslation = translation;
}

/**
 * @brief Transform::getRotation
 * @return
 */
glm::vec3 Transform::getRotation() const
{
    return mRotation;
}

/**
 * @brief Transform::setRotation
 * @param rotation
 */
void Transform::setRotation(const glm::vec3& rotation)
{
    mRotation = rotation;
}

/**
 * @brief Transform::getScale
 * @return
 */
glm::vec3 Transform::getScale() const
{
    return mScale;
}

/**
 * @brief Transform::setScale
 * @param scale
 */
void Transform::setScale(const glm::vec3& scale)
{
    mScale = scale;
}
