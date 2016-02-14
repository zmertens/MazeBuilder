#ifndef UTILS_HPP
#define UTILS_HPP

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

} // namespace Utils

#endif // UTILS_HPP
