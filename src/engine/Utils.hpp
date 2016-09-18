#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <sstream>
#include <random>

#include <glm/glm.hpp>

namespace Utils
{
template <typename T>
inline std::string toString(const T& in)
{
    std::stringstream ss;
    ss << in;
    return ss.str();
}

/**
 * @brief getRandomFloat
 * @param low
 * @param high
 * @return
 */
inline float getRandomFloat(float low, float high)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<float> dist (low, high);
    return dist(mt);
}

/**
 * @brief getRandomInt
 * @param low
 * @param high
 * @return
 */
inline float getRandomInt(int low, int high)
{
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist (low, high);
    return dist(mt);
}

/**
 * @brief getTexAtlasOffset
 * @param index
 * @param numRows
 * @return
 */
inline glm::vec2 getTexAtlasOffset(const unsigned int index, const unsigned int numRows)
{
    float column = glm::mod(static_cast<float>(index), static_cast<float>(numRows));
    float row = glm::floor(static_cast<float>(index) / static_cast<float>(numRows));
    return glm::vec2(column / numRows, row / numRows);
}

/**
 * Returns zero vector if no movement whatsoever, else it returns 1 along the axis of movement (Y- axis is not checked).
 */
inline glm::vec3 
tileCollision(const glm::vec3& tile, const glm::vec3& tileScale, const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& pScale)
{
    glm::vec3 result (0.0f, 1.0f, 0.0f);

    if (dir.x + pScale.x < tile.x * tileScale.x || dir.x - pScale.x > (tile.x + 1.0f) * tileScale.x  || origin.z + pScale.z < tile.z * tileScale.z || origin.z - pScale.z > (tile.z + 1.0f) * tileScale.z)
    {
        result.x = 1.0f;
    }

    if (origin.x + pScale.x < tile.x * tileScale.x  || origin.x - pScale.x > (tile.x + 1.0f) * tileScale.x  || dir.z + pScale.z < tile.z * tileScale.z || dir.z - pScale.z > (tile.z + 1.0f) * tileScale.z)
    {
        result.z = 1.0f;
    }

    return result;
}

inline glm::vec3 
collision(const std::vector<glm::vec3>& tiles, const glm::vec3& tileScale, const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& pScale)
{
    glm::vec3 v (1);
    for (auto& t : tiles)
    {
        v *= tileCollision(t, tileScale, origin, dir, pScale);
        if (v.x == 0.0f && v.z == 0.0f)
            break;
    }

    return v;
}

} // namespace Utils

#endif // UTILS_HPP
