#ifndef VIEW_HPP
#define VIEW_HPP

#include <SDL3/SDL.h>

class View {
public:
    View();

    void setCenter(float x, float y);
    void setSize(float width, float height);

    const SDL_FRect& getViewport() const;

private:
    SDL_FRect m_viewport;
};

#endif // VIEW_HPP
