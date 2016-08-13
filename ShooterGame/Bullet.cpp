#include "Bullet.hpp"

/**
 * @brief Bullet::Bullet
 * @param maxDistance
 * @param position
 * @param rotation
 * @param scale
 */
Bullet::Bullet(const float maxDistance, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
: MAX_DISTANCE(MAX_DISTANCE)
{

}

void Bullet::update()
{
//    if(mIsActive) {
//        const float dt = mClock.getElapsedTime().asSeconds() - mFireTime;
//        const glm::vec3 lerp (mStartPoint * (1.0f - dt) + mEndPoint * dt);

//        if(glm::ceil(lerp) == glm::ceil(mEndPoint)) {
//            mIsActive = false;
//        }

//        mTransform->setTranslation(lerp);
//        mBulletMesh->updateAABB(mTransform->getModelMatrix());
//    }
}

void Bullet::activate(const glm::vec3& position, const glm::vec3& direction) noexcept
{
//    mIsActive = true;
//    mFireTime = mClock.getElapsedTime().asSeconds();
//    mStartPoint = position;
//    mEndPoint = position + (direction * MAX_DISTANCE);
}

void Bullet::deactivate() noexcept
{
    mIsActive = false;
}

bool Bullet::isActive() const
{
    return mIsActive;
}
