#ifndef ENEMY_HPP
#define ENEMY_HPP

#include "engine/graphics/Sprite.hpp"

class Enemy : public Sprite
{
public:
    typedef std::unique_ptr<Enemy> Ptr;
public:
    explicit Enemy(const Entity::Config& config,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));

};

#endif // ENEMY_HPP
