#ifndef ENEMY_HPP
#define ENEMY_HPP

#include <array>

#include <glm/glm.hpp>

#include "engine/graphics/IDrawable.hpp"
#include "engine/graphics/Sprite.hpp"

class Player;
class Level;

class Enemy : public Sprite
{
public:
    typedef std::unique_ptr<Enemy> Ptr;

    enum class States {
        Idle,
        Attack,
        Hurt,
        Follow,
        Dead
    };

public:
    explicit Enemy(
        const Draw::Config& config,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));
    virtual void update(float dt, double timeSinceInit) override;
    virtual void cleanUp() override;

    Enemy::States getState() const;
    void setState(Enemy::States state);

    float getHealth() const;
    void setHealth(const float health);
    void handleMovement(const float dt, Player& player, const Level& level);
    void inflictDamage(const float min, const float max);

private:
    static constexpr float sAnimFreq = 0.42f;
    static constexpr float sAgroRange = 15.0f;
    static constexpr float sMinDamage = 0.3f;
    static constexpr float sMaxDamage = 1.0f;

    static const float sEnemySize;
    static const float sMvFactor;

    float mHealth;
    std::array<glm::vec2, 12> mAnimations;
    States mState;
    float mAnimationCounter;
    unsigned int mAnimationIndex;
private:
    void updateAnimations();
    void genAnimations();
    void moveTowardsPlayer(float dt, const Player& player, const Level& level);
};

#endif // ENEMY_HPP
