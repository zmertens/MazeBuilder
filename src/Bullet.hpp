#ifndef BULLET_HPP
#define BULLET_HPP

#include <vector>
#include <memory>
#include <chrono>

#include <glm/glm.hpp>

#include "engine/Transform.hpp"
#include "engine/BoundingBox.hpp"

class Bullet
{
    public:
        typedef std::unique_ptr<Bullet> Ptr;
    public:
        explicit Bullet(const float maxDistance, const glm::vec3& position = glm::vec3(0.0f),
                        const glm::vec3& rotation = glm::vec3(0.0f),
                        const glm::vec3& scale = glm::vec3(1.0f));
        void update();
//        void draw(const glm::mat4& view, const std::unique_ptr<GLSLProgram>& levelShader) const;
        void activate(const glm::vec3& position, const glm::vec3& direction) noexcept;
        void deactivate() noexcept;
        bool isActive() const;
    private:
        // entity config?
        const float MAX_DISTANCE;
        std::chrono::high_resolution_clock mClock;
        std::unique_ptr<Transform> mTransform;
        bool mIsActive;
        glm::vec3 mStartPoint;
        glm::vec3 mEndPoint;
        float mFireTime;
};

#endif // BULLET_HPP
