#include "Light.hpp"

/**
 * @brief Light::Light
 */
Light::Light()
: mAmbient(glm::vec3(0))
, mDiffuse(glm::vec3(0))
, mSpecular(glm::vec3(0))
, mPosition(glm::vec4(0))
{

}

/**
 * @brief Light::Light
 * @param ambient
 * @param diffuse
 * @param specular
 * @param position
 */
Light::Light(const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular,
    const glm::vec4& position)
: mAmbient(ambient)
, mDiffuse(diffuse)
, mSpecular(specular)
, mPosition(position)
{

}

/**
 * @brief Light::getAmbient
 * @return
 */
glm::vec3 Light::getAmbient() const
{
    return mAmbient;
}

/**
 * @brief Light::setAmbient
 * @param ambient
 */
void Light::setAmbient(const glm::vec3& ambient)
{
    mAmbient = ambient;
}

/**
 * @brief Light::getDiffuse
 * @return
 */
glm::vec3 Light::getDiffuse() const
{
    return mDiffuse;
}

/**
 * @brief Light::setDiffuse
 * @param diffuse
 */
void Light::setDiffuse(const glm::vec3& diffuse)
{
    mDiffuse = diffuse;
}

/**
 * @brief Light::getSpecular
 * @return
 */
glm::vec3 Light::getSpecular() const
{
    return mSpecular;
}

/**
 * @brief Light::setSpecular
 * @param specular
 */
void Light::setSpecular(const glm::vec3& specular)
{
    mSpecular = specular;
}

/**
 * @brief Light::getPosition
 * @return
 */
glm::vec4 Light::getPosition() const
{
    return mPosition;
}

/**
 * @brief Light::setPosition
 * @param position
 */
void Light::setPosition(const glm::vec4& position)
{
    mPosition = position;
}
