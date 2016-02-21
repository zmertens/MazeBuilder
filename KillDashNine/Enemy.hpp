#ifndef ENEMY_HPP
#define ENEMY_HPP

#include <array>

#include "engine/graphics/Sprite.hpp"

class Enemy : public Sprite
{
public:
    typedef std::unique_ptr<Enemy> Ptr;

    enum class States {
        STATIONARY,
        ATTACKING,
        DYING,
        DEAD
    };

public:
    explicit Enemy(const glm::vec3& scalar,
        const Entity::Config& config,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));
    virtual void update(float dt, double timeSinceInit) override;

private:
    std::array<glm::vec2, 4> mAnimations;
    States mStates;
    float mAnimationCounter;
    unsigned int mAnimationIndex;
private:
    void updateAnimations();
    void genAnimations();
};

#endif // ENEMY_HPP
