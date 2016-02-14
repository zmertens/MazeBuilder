#include "SpotLight.hpp"

/**
 * @brief SpotLight::SpotLight
 * @param ambient
 * @param diffuse
 * @param specular
 * @param position
 * @param direction
 * @param cutoff
 * @param constant
 * @param linear
 * @param quadratic
 */
SpotLight::SpotLight(const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular,
    const glm::vec4& position, const glm::vec3& direction, float cutoff,
    float constant, float linear, float quadratic)
: Light(ambient, diffuse, specular, position)
, mDirection(direction)
, mCutOff(cutoff)
, mAttenuation(constant, linear, quadratic)
{

}

/**
 * @brief SpotLight::getDirection
 * @return
 */
glm::vec3 SpotLight::getDirection() const
{
    return mDirection;
}

/**
 * @brief SpotLight::setDirection
 * @param direction
 */
void SpotLight::setDirection(const glm::vec3& direction)
{
    mDirection = direction;
}

/**
 * @brief SpotLight::getCutOff
 * @return
 */
float SpotLight::getCutOff() const
{
    return mCutOff;
}

/**
 * @brief SpotLight::setCutOff
 * @param cutoff
 */
void SpotLight::setCutOff(float cutoff)
{
    mCutOff = cutoff;
}

/**
 * @brief SpotLight::getAttenuation
 * @return
 */
Attenuation SpotLight::getAttenuation() const
{
    return mAttenuation;
}
