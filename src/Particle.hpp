#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <memory>

#include <glm/glm.hpp>

#include "engine/graphics/IDrawable.hpp"
#include "engine/Transform.hpp"
#include "engine/SdlWindow.hpp"

class Player;

class Particle : public IDrawable
{
public:
    using Ptr = std::unique_ptr<Particle>;
public:
    explicit Particle(const Player& player, const Draw::Config& config,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));
    virtual void update(float dt, double timeSinceInit) override;
    virtual void draw(const SdlWindow& sdlManager, ResourceManager& rm, const Camera& camera, const IMesh::Draw type = IMesh::Draw::TRIANGLES) const override;
    virtual void cleanUp() override;

    Transform getTransform() const;
    void setTransform(const Transform& transform);

protected:
    Draw::Config mConfig;
    Transform mTransform;
    float mTimePassed;
    float mDelta;

private:
    static constexpr GLuint nParticles = 10000;

    const Player& mPlayer;

    GLuint particleArray[2];
    GLuint feedback[2];

    GLuint posBuf[2];
    GLuint velBuf[2];
    GLuint startTime[2];
    GLuint initVel;

    bool drawBuf;

private:
    void initBuffers();
};

#endif // PARTICLE_HPP
