#include "Skybox.hpp"

#include "../ResourceManager.hpp"
#include "../Camera.hpp"
#include "../SdlWindow.hpp"

/**
 * @brief Skybox::Skybox
 * @param config
 */
Skybox::Skybox(const Draw::Config& config)
: mConfig(config)
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

    auto& shader = rm.getShader(mConfig.shaderId);
    shader->bind();

    auto& tex = rm.getTexture(mConfig.textureId);
    tex->bind();

    shader->setUniform("uViewMatrix", camera.getLookAt());

    auto& mesh = rm.getMesh(mConfig.meshId);

    glDepthFunc(GL_LEQUAL);
    glClearBufferfv(GL_COLOR, 0, gray);
    glClearBufferfv(GL_DEPTH, 0, ones);
    glDisable(GL_DEPTH_TEST);

    mesh->draw(type);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void Skybox::cleanUp()
{

}