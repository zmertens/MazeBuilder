#ifndef DRAWABLE_HPP
#define DRAWABLE_HPP

struct OrthographicCamera;
struct SDL_Renderer;

#include <memory>

class Drawable {
public:
    virtual void draw(SDL_Renderer* renderer, 
        std::unique_ptr<OrthographicCamera> const& camera,
        float pixelsPerMeter, float offsetX, float offsetY, float cellSize, int display_w, int display_h) const = 0;
};

#endif // DRAWABLE_HPP
