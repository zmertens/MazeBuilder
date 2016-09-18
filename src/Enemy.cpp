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
, mHealth(100.0f)
, mStates(States::Sit)
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

    if (mHealth < 0.0f)
        mStates = States::Dead;
}

float Enemy::getHealth() const
{
    return mHealth;
}

void Enemy::setHealth(const float health)
{
    mHealth = health;
}

Enemy::States Enemy::getState() const
{
    return mStates;
}

void Enemy::setState(Enemy::States state)
{
    mStates = state;
}

void Enemy::inflictDamage(const float min, const float max)
{
    mHealth -= Utils::getRandomFloat(min, max);
}


void Enemy::updateAnimations()
{
    if (mStates == States::Sit)
    {
        mConfig.front().texOffset0 = mAnimations[mAnimationIndex];
        mAnimationIndex += 1;
        if (mAnimationIndex >= 4)
            mAnimationIndex = 0;
    }
    else if (mStates == States::Attack)
    {
        mConfig.front().texOffset0 = mAnimations[mAnimationIndex];
        mAnimationIndex += 1;
        if (mAnimationIndex >= 8)
            mAnimationIndex = 4;
    }
    else if (mStates == States::Attack)
    {
        mConfig.front().texOffset0 = mAnimations[mAnimationIndex];
        mAnimationIndex += 1;
        if (mAnimationIndex >= 12)
            mAnimationIndex = 8;
    }
    else
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

    mAnimations[RPG_1_BACK_1] = Utils::getTexAtlasOffset(RPG_1_BACK_1, TEST_RPG_CHARS_NUM_ROWS);
    mAnimations[RPG_1_BACK_2] = Utils::getTexAtlasOffset(RPG_1_BACK_2, TEST_RPG_CHARS_NUM_ROWS);
    mAnimations[RPG_1_BACK_3] = Utils::getTexAtlasOffset(RPG_1_BACK_3, TEST_RPG_CHARS_NUM_ROWS);
    mAnimations[RPG_1_BACK_4] = Utils::getTexAtlasOffset(RPG_1_BACK_4, TEST_RPG_CHARS_NUM_ROWS);

    mAnimations[RPG_1_FRONT_1] = Utils::getTexAtlasOffset(RPG_1_FRONT_1, TEST_RPG_CHARS_NUM_ROWS);
    mAnimations[RPG_1_FRONT_2] = Utils::getTexAtlasOffset(RPG_1_FRONT_2, TEST_RPG_CHARS_NUM_ROWS);
    mAnimations[RPG_1_FRONT_3] = Utils::getTexAtlasOffset(RPG_1_FRONT_3, TEST_RPG_CHARS_NUM_ROWS);
    mAnimations[RPG_1_FRONT_4] = Utils::getTexAtlasOffset(RPG_1_FRONT_4, TEST_RPG_CHARS_NUM_ROWS);
}
