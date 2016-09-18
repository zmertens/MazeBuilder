#ifndef ENTITY
#define ENTITY

#include <string>
#include <memory>
#include <list>

#include <glm/glm.hpp>

#include "IndexedMeshImpl.hpp"
#include "../Transform.hpp"

class ResourceManager;
class Camera;
class SdlWindow;

/**
 * @brief The Entity class
 */
class Entity
{
public:
    typedef std::unique_ptr<Entity> Ptr;

    /**
     * Abstracts the Entity's associated drawing data.
     * offset2 = normal map texture usually
     * @brief The Config struct
     */
    struct Config {
        std::string shaderId;
        std::string meshId;
        std::string materialId;
        std::string textureId;
        glm::vec2 texOffset0;
        glm::vec2 texOffset1;
        glm::vec2 texOffset2;

        Config(const std::string& shader = std::string(""),
            const std::string& mesh = std::string(""),
            const std::string& material = std::string(""),
            const std::string& tex = std::string(""),
            const glm::vec2& offset0 = glm::vec2(-1.0f),
            const glm::vec2& offset1 = glm::vec2(-1.0f),
            const glm::vec2& offset2 = glm::vec2(-1.0f))
        : shaderId(shader)
        , meshId(mesh)
        , materialId(material)
        , textureId(tex)
        , texOffset0(offset0)
        , texOffset1(offset1)
        , texOffset2(offset2)
        {

        }

        inline bool hasOffset(const glm::vec2& test) const;
    };

public:
    explicit Entity(const Entity::Config& config,
        const glm::vec3& position = glm::vec3(0.0f),
        const glm::vec3& rotation = glm::vec3(0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));

    virtual void update(float dt, double timeSinceInit);
    virtual void draw(const SdlWindow& sdlManager,
        ResourceManager& rm,
        const Camera& camera,
        const IMesh::Draw type = IMesh::Draw::TRIANGLES) const;

    Transform getTransform() const;
    void setTransform(const Transform& transform);

protected:
    std::list<Entity::Config> mConfig;
    Transform mTransform;
    float mCounter;

private:

};

/**
 * @brief Entity::Config::hasOffset
 * @param test
 * @return
 */
inline bool Entity::Config::hasOffset(const glm::vec2& test) const
{
    return test.x != -1.0f && test.y != -1.0f;
}

#endif // ENTITY

