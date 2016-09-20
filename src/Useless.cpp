#include "Useless.hpp"

#include <glm/gtc/constants.hpp>

#include "engine/ResourceManager.hpp"
#include "engine/Camera.hpp"
#include "engine/SdlWindow.hpp"

/**
 * @brief Useless::Useless
 * @param config
 * @param position = glm::vec3(0.0f)
 * @param rotation = glm::vec3(0.0f)
 * @param scale = glm::vec3(1.0f)
 */
Useless::Useless(const Draw::Config& config,
    const glm::vec3& position,
    const glm::vec3& rotation,
    const glm::vec3& scale)
: mConfig(config)
, mTransform(position, rotation, scale)
, mCounter(0.0f)
{

}

/**
 * @brief Useless::update
 * @param dt
 * @param timeSinceInit
 */
void Useless::update(float dt, double timeSinceInit)
{
    mCounter += glm::two_pi<float>() / dt;
    if (mCounter > glm::two_pi<float>())
        mCounter -= glm::two_pi<float>();

    mTransform.setRotation(glm::vec3(mCounter * glm::radians(0.15f),
        mCounter * glm::radians(0.25f),
        0));
}

/**
 * @brief Useless::draw
 * @param sdlManager
 * @param rm
 * @param camera
 * @param type = IMesh::Draw::TRIANGLES
 */
void Useless::draw(const SdlWindow& sdlManager,
    ResourceManager& rm,
    const Camera& camera,
    const IMesh::Draw type) const
{
    auto& shader = rm.getShader(mConfig.shaderId);
    shader->bind();

    auto& tex = rm.getTexture(mConfig.textureId);
    tex->bind();

    auto mv = mTransform.getModelView(camera.getLookAt());
    auto persp = camera.getPerspective(sdlManager.getAspectRatio());
    shader->setUniform("uProjMatrix", persp);
    shader->setUniform("uModelViewMatrix", mv);

    auto& mesh = rm.getMesh(mConfig.meshId);
    mesh->draw(type, 1);
} // draw

void Useless::cleanUp()
{

}

Transform Useless::getTransform() const
{
    return mTransform;
}

void Useless::setTransform(const Transform& transform)
{
    mTransform = transform;
}
