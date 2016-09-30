#include "Enemy.hpp"

#include "ResourceConstants.hpp"
#include "Level.hpp"
#include "Player.hpp"
#include "Power.hpp"

#include "engine/Utils.hpp"

const float Enemy::sEnemySize = 0.5f;
const float Enemy::sMvFactor = 17.5f;

/**
 * @brief Enemy::Enemy
 * @param config
 * @param position = glm::vec3(0.0f)
 * @param rotation = glm::vec3(0.0f)
 * @param scale = glm::vec3(1.0f)
 */
Enemy::Enemy(
    const Draw::Config& config,
    const glm::vec3& position,
    const glm::vec3& rotation,
    const glm::vec3& scale)
: Sprite(config, position, rotation, scale)
, mHealth(100.0f)
, mState(States::Idle)
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

    if (mAnimationCounter > sAnimFreq)
    {
        mAnimationCounter = 0.0f;

        updateAnimations();
    }

    if (mHealth < 0.0f)
        mState = States::Dead;
}

void Enemy::cleanUp()
{

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
    return mState;
}

void Enemy::setState(Enemy::States state)
{
    mState = state;
}

void Enemy::handleMovement(const float dt, Player& player, const Level& level)
{
    bool inRange = glm::length(player.getPosition() - mTransform.getTranslation()) < level.getSpriteHalfWidth();

    if (mState == Enemy::States::Attack)
    {
        moveTowardsPlayer(dt, player, level);
        if (inRange)
            player.inflictDamage();
    }

    if (inRange && player.isShooting())
    {
        if (player.getPower() == Power::Type::Strength)
            this->inflictDamage(sMinDamage + 0.5f, sMaxDamage + 0.5f);
        else
            this->inflictDamage(sMinDamage, sMaxDamage);
    }

    if (mState == Enemy::States::Idle && glm::length(mTransform.getTranslation() - player.getPosition()) < sAgroRange)
    {
        mState = Enemy::States::Attack;
    }

}

void Enemy::inflictDamage(const float min, const float max)
{
    mHealth -= Utils::getRandomFloat(min, max);
}


void Enemy::updateAnimations()
{
    if (mState == States::Idle)
    {
        mConfig.texAtlasOffset = mAnimations[mAnimationIndex];
        mAnimationIndex += 1;
        if (mAnimationIndex > 2)
            mAnimationIndex = 0;
    }
    else if (mState == States::Attack)
    {
        mConfig.texAtlasOffset = mAnimations[mAnimationIndex];
        mAnimationIndex += 1;
        if (mAnimationIndex > 5)
            mAnimationIndex = 3;
    }
    else if (mState == States::Hurt)
    {
        mConfig.texAtlasOffset = mAnimations[mAnimationIndex];
        mAnimationIndex += 1;
        if (mAnimationIndex > 8)
            mAnimationIndex = 6;
    }
    else if (mState == States::Dead)
    {
        mConfig.texAtlasOffset = mAnimations[mAnimationIndex];
        mAnimationIndex += 1;
        if (mAnimationIndex > 11)
            mAnimationIndex = 9;
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

    // idle
    mAnimations[IDLE_0] = Utils::getTexAtlasOffset(IDLE_0, ENEMY_ATLAS_TEX_NUM_ROWS);
    mAnimations[IDLE_1] = Utils::getTexAtlasOffset(IDLE_1, ENEMY_ATLAS_TEX_NUM_ROWS);
    mAnimations[IDLE_2] = Utils::getTexAtlasOffset(IDLE_2, ENEMY_ATLAS_TEX_NUM_ROWS);
    // attack
    mAnimations[ATTACK_0] = Utils::getTexAtlasOffset(ATTACK_0, ENEMY_ATLAS_TEX_NUM_ROWS);
    mAnimations[ATTACK_1] = Utils::getTexAtlasOffset(ATTACK_1, ENEMY_ATLAS_TEX_NUM_ROWS);
    mAnimations[ATTACK_2] = Utils::getTexAtlasOffset(ATTACK_2, ENEMY_ATLAS_TEX_NUM_ROWS);
    // hurt
    mAnimations[HURT_0] = Utils::getTexAtlasOffset(HURT_0, ENEMY_ATLAS_TEX_NUM_ROWS);
    mAnimations[HURT_1] = Utils::getTexAtlasOffset(HURT_1, ENEMY_ATLAS_TEX_NUM_ROWS);
    mAnimations[HURT_2] = Utils::getTexAtlasOffset(HURT_2, ENEMY_ATLAS_TEX_NUM_ROWS);
    // dead
    mAnimations[DEAD_0] = Utils::getTexAtlasOffset(DEAD_0, ENEMY_ATLAS_TEX_NUM_ROWS);
    mAnimations[DEAD_1] = Utils::getTexAtlasOffset(DEAD_1, ENEMY_ATLAS_TEX_NUM_ROWS);
    mAnimations[DEAD_2] = Utils::getTexAtlasOffset(DEAD_2, ENEMY_ATLAS_TEX_NUM_ROWS);
}

void Enemy::moveTowardsPlayer(float dt, const Player& player, const Level& level)
{
    glm::vec3 finalMovement = glm::normalize(player.getPosition() - mTransform.getTranslation());
    glm::vec3 origin = mTransform.getTranslation();
    // R(t) = P + Vt
    glm::vec3 direction (origin + glm::normalize(finalMovement * dt));
    glm::vec3 c (Utils::collision(level.getEmptySpace(), level.getTileScalar(), origin, direction, glm::vec3(sEnemySize)));
    finalMovement *= c;
    finalMovement.y = 0.0f;

    mTransform.setTranslation(origin + finalMovement * dt * sMvFactor);
}
