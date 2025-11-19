#ifndef VIEW_HPP
#define VIEW_HPP

#include <SDL3/SDL.h>

class View
{
public:
    View();

    void setCenter(float x, float y);
    void setSize(float width, float height);
    void zoom(float factor);
    void move(float offsetX, float offsetY);
    void rotate(float angle);

    SDL_FPoint getCenter() const;
    SDL_FPoint getSize() const;
    float getRotation() const;

private:
    SDL_FPoint m_center;
    SDL_FPoint m_size;
    float m_rotation; // in degrees
};

#endif // VIEW_HPP
