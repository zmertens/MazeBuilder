#ifndef BOUNDINGBOX_HPP
#define BOUNDINGBOX_HPP

#include <glm/glm.hpp>

class BoundingBox
{
public:
    BoundingBox(const glm::vec3& min = glm::vec3(0), const glm::vec3& max = glm::vec3(0))
    : minCoord(min)
    , maxCoord(max)
    , center(glm::vec3((max.x + min.x) * 0.5f, (max.y + min.y) * 0.5f, (max.z + min.z) * 0.5f))
    {

    }

    inline bool intersects(const BoundingBox& other) const;
    inline bool intersects(const glm::vec3& point) const;
public:
    glm::vec3 minCoord;
    glm::vec3 maxCoord;
    glm::vec3 center;
};

inline bool BoundingBox::intersects(const BoundingBox& other) const
{
    bool intersectX = this->maxCoord.x >= other.minCoord.x && other.maxCoord.x >= this->minCoord.x;
    bool intersectY = this->maxCoord.y >= other.minCoord.y && other.maxCoord.y >= this->minCoord.y;
    bool intersectZ = this->maxCoord.z >= other.minCoord.z && other.maxCoord.z >= this->minCoord.z;

    return intersectX && intersectY && intersectZ;
}

inline bool BoundingBox::intersects(const glm::vec3& point) const
{
    bool intersectX = this->maxCoord.x >= point.x && point.x >= this->minCoord.x;
    bool intersectY = this->maxCoord.y >= point.y && point.y >= this->minCoord.y;
    bool intersectZ = this->maxCoord.z >= point.z && point.z >= this->minCoord.z;

    return intersectX && intersectY && intersectZ;
}

#endif // BOUNDINGBOX_HPP
