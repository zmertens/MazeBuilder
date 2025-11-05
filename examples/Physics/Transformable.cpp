#include "Transformable.hpp"

Transformable::Transformable()
    : mPosition{0.f, 0.f}
      , mRotation{0.f}
      , mScale{1.f, 1.f}
{
}

void Transformable::setPosition(float x, float y)
{
    mPosition.x = x;
    mPosition.y = y;
}

void Transformable::setPosition(const b2Vec2& position)
{
    mPosition = position;
}

void Transformable::move(float offsetX, float offsetY)
{
    mPosition.x += offsetX;
    mPosition.y += offsetY;
}

void Transformable::move(const b2Vec2& offset)
{
    mPosition += offset;
}

const b2Vec2& Transformable::getPosition() const
{
    return mPosition;
}

void Transformable::setRotation(b2Rot angle)
{
    mRotation = angle;
}

void Transformable::rotate(float angle)
{
    mRotation.c = angle;
}

b2Rot Transformable::getRotation() const
{
    return mRotation;
}

void Transformable::setScale(float factorX, float factorY)
{
    mScale.x = factorX;
    mScale.y = factorY;
}

void Transformable::setScale(const b2Vec2& factors)
{
    mScale = factors;
}

void Transformable::scale(float factorX, float factorY)
{
    mScale.x *= factorX;
    mScale.y *= factorY;
}

void Transformable::scale(const b2Vec2& factor)
{
    mScale.x *= factor.x;
    mScale.y *= factor.y;
}

const b2Vec2& Transformable::getScale() const
{
    return mScale;
}
