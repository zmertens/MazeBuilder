#include "Particle.hpp"

#include "engine/SdlWindow.hpp"

const float Particle::scMaxDistance = 100.0f;

/**
 * @brief Particle::Particle
 * @param position
 * @param dir
 */
Particle::Particle(const glm::vec3& position, const glm::vec3& dir)
: mPosition(position)
, mActive(true)
, mStartPoint(position)
, mEndPoint(position + (dir * scMaxDistance))
, mFireTime(static_cast<double>(SDL_GetTicks()) / 1000.0)
{
	update();
}

void Particle::update()
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

bool Particle::isActive() const
{
    return mActive;
}

bool Particle::intersects(const glm::vec3& other, float size) const
{
	return glm::length(mPosition - other) < size;
}