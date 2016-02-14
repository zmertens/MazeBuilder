#ifndef POINTLIGHT_HPP
#define POINTLIGHT_HPP

#include <memory>

#include "Light.hpp"
#include "Attenuation.hpp"

class PointLight : public Light
{
public:
    typedef std::unique_ptr<PointLight> Ptr;
public:
    PointLight();
    explicit PointLight(const glm::vec3& ambient, const glm::vec3& diffuse, const glm::vec3& specular,
        const glm::vec4& position, float range,
        float constant, float linear, float quadratic);

    float getRange() const;
    void setRange(float range);

    Attenuation getAttenuation() const;
    void setAttenuation(const Attenuation& attenuation);

private:
    float mRange;
    Attenuation mAttenuation;
};

#endif // POINTLIGHT_HPP
