#ifndef POWERUP_HPP
#define POWERUP_HPP

//#include "engine/graphics/Sprite.hpp"
//#include "engine/graphics/PostProcessorImpl.hpp"

namespace PowerDurations
{
const float SPEED = 60u;
const float RECHARGING = 45u;
const float INVINCIBILITY = 25u;
}

//class PowerUp : public Sprite
//{
//public:
//    typedef std::unique_ptr<PowerUp> Ptr;

//public:
//    explicit PowerUp(Effects::Type type,
//        const float duration,
//        const Entity::Config& config,
//        const glm::vec3& position = glm::vec3(0.0f),
//        const glm::vec3& rotation = glm::vec3(0.0f),
//        const glm::vec3& scale = glm::vec3(1.0f));

//private:
//    Effects::Type mType;
//    float mDuration;

//};

#endif // POWERUP_HPP
