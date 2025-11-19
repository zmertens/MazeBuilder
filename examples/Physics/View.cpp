#include "View.hpp"

View::View() : m_center({0.f, 0.f}), m_size({0.f, 0.f}), m_rotation(0.f)
{
}

void View::setCenter(float x, float y)
{
    m_center = {x, y};
}

void View::setSize(float width, float height)
{
    m_size = {width, height};
}

void View::zoom(float factor)
{
    m_size.x *= factor;
    m_size.y *= factor;
}

void View::move(float offsetX, float offsetY)
{
    m_center.x += offsetX;
    m_center.y += offsetY;
}

void View::rotate(float angle)
{
    m_rotation += angle;
}

SDL_FPoint View::getCenter() const
{
    return m_center;
}

SDL_FPoint View::getSize() const
{
    return m_size;
}

float View::getRotation() const
{
    return m_rotation;
}
