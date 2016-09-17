#ifndef SPRITE_HPP
#define SPRITE_HPP

#include "Entity.hpp"

class Sprite : public Entity
{
public:
    explicit Sprite(const Entity::Config& config,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));
    virtual void update(float dt, double timeSinceInit) override;
    virtual void draw(const SdlWindow& sdlManager,
        ResourceManager& rm,
        const Camera& camera,
        const IMesh::Draw type = IMesh::Draw::TRIANGLES) const override;
};

#endif // SPRITE_HPP
