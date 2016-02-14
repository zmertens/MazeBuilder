#include "Fog.hpp"

/**
 * @brief Fog::Fog
 */
Fog::Fog()
: mColor(glm::vec3(0.5f))
, mMaxDistance(100.0f)
, mMinDistance(10.0f)
{

}

/**
 * @brief Fog::Fog
 * @param color
 * @param max
 * @param min
 */
Fog::Fog(const glm::vec3& color, float max, float min)
: mColor(color)
, mMaxDistance(max)
, mMinDistance(min)
{

}

/**
 * @brief Fog::getColor
 * @return
 */
glm::vec3 Fog::getColor() const
{
    return mColor;
}

/**
 * @brief Fog::setColor
 * @param color
 */
void Fog::setColor(const glm::vec3& color)
{
    mColor = color;
}

/**
 * @brief Fog::getMaxDistance
 * @return
 */
float Fog::getMaxDistance() const
{
    return mMaxDistance;
}

/**
 * @brief Fog::setMaxDistance
 * @param max
 */
void Fog::setMaxDistance(float max)
{
    mMaxDistance = max;
}

/**
 * @brief Fog::getMinDistance
 * @return
 */
float Fog::getMinDistance() const
{
    return mMinDistance;
}

/**
 * @brief Fog::setMinDistance
 * @param min
 */
void Fog::setMinDistance(float min)
{
    mMinDistance = min;
}
