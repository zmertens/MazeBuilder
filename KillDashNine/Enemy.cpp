#include "Enemy.hpp"

/**
 * @brief Enemy::Enemy
 * @param config
 * @param position = glm::vec3(0.0f)
 * @param rotation = glm::vec3(0.0f)
 * @param scale = glm::vec3(1.0f)
 */
Enemy::Enemy(const Entity::Config& config,
    const glm::vec3& position,
    const glm::vec3& rotation,
    const glm::vec3& scale)
: Sprite(config, position, rotation, scale)
{

}
