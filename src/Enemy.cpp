#include "Enemy.hpp"

#include "ResourceConstants.hpp"

#include "engine/Utils.hpp"

/**
 * @brief Enemy::Enemy
 * @param scalar
 * @param config
 * @param position = glm::vec3(0.0f)
 * @param rotation = glm::vec3(0.0f)
 * @param scale = glm::vec3(1.0f)
 */
Enemy::Enemy(const glm::vec3& scalar,
    const Entity::Config& config,
    const glm::vec3& position,
    const glm::vec3& rotation,
    const glm::vec3& scale)
: Sprite(config, position, rotation, scale)
, mStates(States::STATIONARY)
, mAnimationCounter(0.0f)
, mAnimationIndex(0u)
{
    genAnimations();
}

/**
 * @brief Enemy::update
 * @param dt
 * @param timeSinceInit
 */
void Enemy::update(float dt, double timeSinceInit)
{
    mAnimationCounter += dt;

    if (mAnimationCounter > 0.35f)
    {
        mAnimationCounter = 0.0f;

        updateAnimations();
    }
}

void Enemy::updateAnimations()
{
    if (mStates == States::STATIONARY)
    {
        mConfig.front().texOffset0 = mAnimations[mAnimationIndex];
        mAnimationIndex += 1;
    }

    if (mAnimationIndex >= 4)
        mAnimationIndex = 0;
}

/**
 * @brief Enemy::genAnimations
 */
void Enemy::genAnimations()
{
    using namespace ResourceIds::Textures::Atlas;
    mAnimations[RPG_1_WALK_1] = Utils::getTexAtlasOffset(RPG_1_WALK_1, TEST_RPG_CHARS_NUM_ROWS);
    mAnimations[RPG_1_WALK_2] = Utils::getTexAtlasOffset(RPG_1_WALK_2, TEST_RPG_CHARS_NUM_ROWS);
    mAnimations[RPG_1_WALK_3] = Utils::getTexAtlasOffset(RPG_1_WALK_3, TEST_RPG_CHARS_NUM_ROWS);
    mAnimations[RPG_1_WALK_4] = Utils::getTexAtlasOffset(RPG_1_WALK_4, TEST_RPG_CHARS_NUM_ROWS);
}
