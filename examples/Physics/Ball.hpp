#ifndef BALL_HPP
#define BALL_HPP

#include <tuple>

#include <box2d/box2d.h>

#include "Drawable.hpp"

/// @file Ball.hpp
/// @class Ball
/// @brief Data class for a ball with physics properties
class Ball : public Drawable {
public:
    explicit Ball(std::tuple<float, float, float> coords, float r, const b2WorldId worldId);

    // Getters
    b2BodyId getBodyId() const;
    b2ShapeId getShapeId() const;
    bool getIsActive() const;
    bool getIsDragging() const;
    bool getIsExploding() const;
    float getExplosionTimer() const;
    std::tuple<float, float, float> getCoords() const;
    float getRadius() const;

    // Setters
    void setBodyId(const b2BodyId& id);
    void setShapeId(const b2ShapeId& id);
    void setIsActive(bool active);
    void setIsDragging(bool dragging);
    void setIsExploding(bool exploding);
    void setExplosionTimer(float timer);
    void setCoords(const std::tuple<float, float, float > & newCoords);
    void setRadius(float newRadius);

    // Overrides
    void draw(SDL_Renderer* renderer, 
        std::unique_ptr<OrthographicCamera> const& camera,
        float pixelsPerMeter, float offsetX, float offsetY, float cellSize, int display_w, int display_h) const override;

private:
    std::tuple<float, float, float> coords;
    float radius;

    b2BodyId bodyId;
    b2ShapeId shapeId;
    bool isActive;
    bool isDragging;
    bool isExploding;
    float explosionTimer;
};

#endif // BALL_HPP
