#ifndef SPOTLIGHT_HPP
#define SPOTLIGHT_HPP

#include <memory>

#include "Light.hpp"
#include "Attenuation.hpp"

/**
 * @brief The SpotLight class
 */
class SpotLight : public Light
{
public:
    typedef std::unique_ptr<SpotLight> Ptr;
public:
    explicit SpotLight(const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular,
        const glm::vec4& position, const glm::vec3& direction, float cutoff,
        float constant, float linear, float quadratic);

    glm::vec3 getDirection() const;
    void setDirection(const glm::vec3& direction);

    float getCutOff() const;
    void setCutOff(float cutOff);

    Attenuation getAttenuation() const;

private:
    glm::vec3 mDirection;
    float mCutOff;
    const Attenuation mAttenuation;
};

#endif // SPOTLIGHT_HPP
