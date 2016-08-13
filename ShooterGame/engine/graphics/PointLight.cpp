#include "PointLight.hpp"

/**
 * @brief PointLight::PointLight
 */
PointLight::PointLight()
: Light(glm::vec3(0), glm::vec3(0), glm::vec3(0), glm::vec4(0))
, mRange(100.0f)
, mAttenuation(1.0f, 1.0f, 1.0f)
{

}

/**
 * @brief PointLight::PointLight
 * @param ambient
 * @param diffuse
 * @param specular
 * @param position
 * @param range
 * @param constant
 * @param linear
 * @param quadratic
 */
PointLight::PointLight(const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular,
    const glm::vec4& position, float range,
    float constant, float linear, float quadratic)
: Light(ambient, diffuse, specular, position)
, mRange(range)
, mAttenuation(constant, linear, quadratic)
{

}

/**
 * @brief PointLight::getRange
 * @return
 */
float PointLight::getRange() const
{
    return mRange;
}

/**
 * @brief PointLight::setRange
 * @param range
 */
void PointLight::setRange(float range)
{
    mRange = range;
}

/**
 * @brief PointLight::getAttenuation
 * @return
 */
Attenuation PointLight::getAttenuation() const
{
    return mAttenuation;
}

/**
 * @brief PointLight::setAttenuation
 * @param attenuation
 */
void PointLight::setAttenuation(const Attenuation& attenuation)
{
    mAttenuation = attenuation;
}
