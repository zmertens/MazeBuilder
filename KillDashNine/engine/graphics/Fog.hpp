#ifndef FOG_HPP
#define FOG_HPP

#include <glm/glm.hpp>

class Fog
{
public:
    Fog();
    explicit Fog(const glm::vec3& color, float max, float min);

    glm::vec3 getColor() const;
    void setColor(const glm::vec3& color);

    float getMaxDistance() const;
    void setMaxDistance(float max);

    float getMinDistance() const;
    void setMinDistance(float min);
private:
    glm::vec3 mColor;
    float mMaxDistance;
    float mMinDistance;
};

#endif // FOG_HPP
