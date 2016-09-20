#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <memory>

#include <glm/glm.hpp>

#include "engine/BoundingBox.hpp"

class Particle
{
public:
    typedef std::unique_ptr<Particle> Ptr;
public:
    explicit Particle(const glm::vec3& position, const glm::vec3& dir);
    void update();
    bool isActive() const;
    bool intersects(const glm::vec3& other, float size) const;
private:
    static const float scMaxDistance;
    glm::vec3 mPosition;
    bool mActive;
    glm::vec3 mStartPoint;
    glm::vec3 mEndPoint;
    double mFireTime;
};

#endif // PARTICLE_HPP
