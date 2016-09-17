#include "Entity.hpp"

#include <glm/gtc/constants.hpp> // 2pi

#include "../ResourceManager.hpp"
#include "../Camera.hpp"
#include "../SdlWindow.hpp"

/**
 * @brief Entity::Entity
 * @param config
 * @param position = glm::vec3(0.0f)
 * @param rotation = glm::vec3(0.0f)
 * @param scale = glm::vec3(1.0f)
 */
Entity::Entity(const Entity::Config& config,
    const glm::vec3& position,
    const glm::vec3& rotation,
    const glm::vec3& scale)
: mTransform(position, rotation, scale)
, mCounter(0.0f)
{
    mConfig.push_back(config);
}

/**
 * @brief Entity::update
 * @param dt
 * @param timeSinceInit
 */
void Entity::update(float dt, double timeSinceInit)
{
    mCounter += glm::two_pi<float>() / dt;
    if (mCounter > glm::two_pi<float>())
        mCounter -= glm::two_pi<float>();

    mTransform.setRotation(glm::vec3(mCounter * glm::radians(0.15f),
        mCounter * glm::radians(0.25f),
        0));
}

/**
 * Shaders and Textures are often bound by batching,
 * however they are still cache checked before drawing.
 *
 * @brief Entity::draw
 * @param sdlManager
 * @param rm
 * @param camera
 * @param type = IMesh::Draw::TRIANGLES
 */
void Entity::draw(const SdlWindow& sdlManager,
    ResourceManager& rm,
    const Camera& camera,
    const IMesh::Draw type) const
{
    // every config in the list has the same shader and texture
    auto& frontConfig = mConfig.front();
    auto& shader = rm.getShader(frontConfig.shaderId);
    if (!rm.isInCache(frontConfig.shaderId, CachePos::Shader))
    {
        rm.putInCache(frontConfig.shaderId, CachePos::Shader);
        shader->bind();
    }

    auto& tex = rm.getTexture(frontConfig.textureId);
    if (!rm.isInCache(frontConfig.textureId, CachePos::Texture))
    {
        rm.putInCache(frontConfig.textureId, CachePos::Texture);
        tex->bind();
    }

    auto mv = mTransform.getModelView(camera.getLookAt());
    auto persp = camera.getPerspective(sdlManager.getAspectRatio());
    shader->setUniform("uProjMatrix", persp);
    shader->setUniform("uModelViewMatrix", mv);

    for (auto& config : mConfig)
    {
        auto& mat = rm.getMaterial(config.materialId);
        auto& mesh = rm.getMesh(config.meshId);

        shader->setUniform("uMaterial.ambient", mat->getAmbient());
        shader->setUniform("uMaterial.diffuse", mat->getDiffuse());
        shader->setUniform("uMaterial.specular", mat->getSpecular());
        shader->setUniform("uMaterial.shininess", mat->getShininess());

        shader->setUniform("uTexOffset0", config.texOffset0);

        mesh->draw(type);
    }
}

/**
 * @brief Entity::getTransform
 * @return
 */
Transform Entity::getTransform() const
{
    return mTransform;
}

/**
 * @brief Entity::setTransform
 * @param transform
 */
void Entity::setTransform(const Transform& transform)
{
    mTransform = transform;
}
