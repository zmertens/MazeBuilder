#include "Bullet.hpp"

#include "engine/SdlWindow.hpp"

const float Bullet::scMaxDistance = 100.0f;

/**
 * @brief Bullet::Bullet
 * @param position
 * @param dir
 */
Bullet::Bullet(const glm::vec3& position, const glm::vec3& dir)
: mPosition(position)
, mActive(true)
, mStartPoint(position)
, mEndPoint(position + (dir * scMaxDistance))
, mFireTime(static_cast<double>(SDL_GetTicks()) / 1000.0)
{
	update();
}

void Bullet::update()
{
   if (mActive) 
   {
       const double dt = (static_cast<double>(SDL_GetTicks()) / 1000.0) - mFireTime;
       //const glm::vec3 lerp (mStartPoint * (1.0f - dt) + mEndPoint * dt);

       glm::vec3 newPos = glm::mix(mStartPoint, mEndPoint, dt);

       if (dt > 1.0) 
       {
           mActive = false;
       }

       mPosition = newPos;
   }
}

bool Bullet::isActive() const
{
    return mActive;
}

bool Bullet::intersects(const glm::vec3& other, float size) const
{
	return glm::length(mPosition - other) < size;
}