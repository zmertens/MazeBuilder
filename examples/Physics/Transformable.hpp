#ifndef TRANSFORMABLE_HPP
#define TRANSFORMABLE_HPP

#include <box2d/math_functions.h>

class Transformable
{
public:
    Transformable();

    void setPosition(float x, float y);
    void setPosition(const b2Vec2& position);
    void move(float offsetX, float offsetY);
    void move(const b2Vec2& offset);
    const b2Vec2& getPosition() const;

    void setRotation(b2Rot angle);
    void rotate(float angle);
    b2Rot getRotation() const;

    void setScale(float factorX, float factorY);
    void setScale(const b2Vec2& factors);
    void scale(float factorX, float factorY);
    void scale(const b2Vec2& factor);
    const b2Vec2& getScale() const;

private:
    b2Vec2 mPosition;
    b2Rot mRotation;
    b2Vec2 mScale;
};

#endif // TRANSFORMABLE_HPP
