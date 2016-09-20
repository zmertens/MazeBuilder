#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <memory>

#include <glm/glm.hpp>

#include "engine/graphics/IDrawable.hpp"
#include "engine/Transform.hpp"

class Particle : public IDrawable
{
public:
    explicit Particle(const Draw::Config& config,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));
    virtual void update(float dt, double timeSinceInit) override;
    virtual void draw(const SdlWindow& sdlManager, ResourceManager& rm, const Camera& camera, const IMesh::Draw type = IMesh::Draw::TRIANGLES) const override;
    virtual void cleanUp() override;

    Transform getTransform() const;
    void setTransform(const Transform& transform);

protected:
    Draw::Config mConfig;
    Transform mTransform;
    float mCounter;
}
#endif // PARTICLE_HPP
