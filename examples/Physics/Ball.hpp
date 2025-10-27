#ifndef BALL_HPP
#define BALL_HPP

#include <tuple>

/// @file Ball.hpp
/// @class Ball
/// @brief Data class for a ball with physics properties
class Ball {
public:
    

    bool getIsActive() const {
        return isActive;
    }
    bool getIsDragging() const {
        return isDragging;
    }
    bool getIsExploding() const {
        return isExploding;
    }
    float getExplosionTimer() const {
        return explosionTimer;
    }
    std::tuple<float, float, float> getCoords() const {
        return coords;
    }
    float getRadius() const {
        return radius;
    }

    void setIsActive(bool active) {
        isActive = active;
    }
    void setIsDragging(bool dragging) {
        isDragging = dragging;
    }
    void setIsExploding(bool exploding) {
        isExploding = exploding;
    }
    void setExplosionTimer(float timer) {
        explosionTimer = timer;
    }
    void setCoords(const std::tuple<float, float, float > & newCoords) {
        coords = newCoords;
    }
    void setRadius(float newRadius) {
        radius = newRadius;
    }

    void update(float elapsed) noexcept;

    void draw(float elapsed) const noexcept;

private:
    std::tuple<float, float, float> coords;
    float radius;

    bool isActive;
    bool isDragging;
    bool isExploding;
    float explosionTimer;
};

#endif // BALL_HPP
