#ifndef IDRAWABLE_HPP
#define IDRAWABLE_HPP

#include <string>
#include <memory>
#include <list>

#include <glm/glm.hpp>

#include "IMesh.hpp"

namespace Draw
{
struct Config {
    std::string shaderId;
    std::string meshId;
    std::string materialId;
    std::string textureId;
    glm::vec2 texAtlasOffset;

    Config(const std::string& shader = std::string(""),
        const std::string& mesh = std::string(""),
        const std::string& material = std::string(""),
        const std::string& tex = std::string(""),
        const glm::vec2& offset = glm::vec2(-1.0f))    
    : shaderId(shader)
    , meshId(mesh)
    , materialId(material)
    , textureId(tex)
    , texAtlasOffset(offset)
    {

    }
};
} // namespace

class SdlWindow;
class ResourceManager;
class Camera;

class IDrawable
{
public:
    typedef std::unique_ptr<IDrawable> Ptr;

public:
    virtual void update(float dt, double timeSinceInit) = 0;
    virtual void draw(const SdlWindow& sdlManager, ResourceManager& rm, const Camera& camera, const IMesh::Draw type = IMesh::Draw::TRIANGLES) const = 0;
    virtual void cleanUp() = 0;
};

#endif // IDRAWABLE_HPP

