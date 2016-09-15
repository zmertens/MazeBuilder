#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <glm/glm.hpp>

class Vertex
{
public:
    Vertex(const glm::vec3& pos = glm::vec3(0),
        const glm::vec2& tex = glm::vec2(0),
        const glm::vec3& norm = glm::vec3(0),
        const glm::vec3& tang = glm::vec3(0))
    : position(pos)
    , texCoord(tex)
    , normal(norm)
    , tangent(tang)
    {

    }

public:
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec3 tangent;
};

#endif // VERTEX_HPP
