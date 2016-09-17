#include "Skybox.hpp"

#include "../ResourceManager.hpp"
#include "../Camera.hpp"
#include "../SdlWindow.hpp"

/**
 * @brief Skybox::Skybox
 * @param config
 * @param position = glm::vec3(0.0f)
 * @param rotation = glm::vec3(0.0f)
 * @param scale = glm::vec3(1.0f)
 */
Skybox::Skybox(const Entity::Config& config,
    const glm::vec3& position,
    const glm::vec3& rotation,
    const glm::vec3& scale)
: Entity(config, position, rotation, scale)
{

}

/**
 * @brief Skybox::update
 * @param dt
 * @param timeSinceInit
 */
void Skybox::update(float dt, double timeSinceInit)
{

}

/**
 * @brief Skybox::draw
 * @param sdlManager
 * @param rm
 * @param camera
 * @param type
 */
void Skybox::draw(const SdlWindow& sdlManager,
    ResourceManager& rm,
    const Camera& camera,
    const IMesh::Draw type) const
{
    static const GLfloat gray[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    static const GLfloat ones[] = { 1.0f };

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

    shader->setUniform("uViewMatrix", camera.getLookAt());

    auto& mesh = rm.getMesh(frontConfig.meshId);

    glDepthFunc(GL_LEQUAL);
    glClearBufferfv(GL_COLOR, 0, gray);
    glClearBufferfv(GL_DEPTH, 0, ones);
    glDisable(GL_DEPTH_TEST);

    mesh->draw(type);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}
