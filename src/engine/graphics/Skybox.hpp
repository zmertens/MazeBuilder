#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include "Entity.hpp"

class Skybox : public Entity
{
public:
    explicit Skybox(const Entity::Config& config,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));
    virtual void update(float dt, double timeSinceInit) override;
    virtual void draw(const SdlWindow& sdlManager,
        ResourceManager& rm,
        const Camera& camera,
        const IMesh::Draw type = IMesh::Draw::TRIANGLES) const override;

private:

};

#endif // SKYBOX_HPP
