#ifndef WALL_H
#define WALL_H

#include <memory>

struct SDL_Renderer;
namespace mazes
{
    class randomizer;
}
class texture;

/// @file wall.h
/// @class wall
/// @brief Data class for a wall with physics properties
class wall
{
public:

    explicit wall(std::unique_ptr<texture> t);

    void draw(SDL_Renderer* renderer) const noexcept;

    void update(float elapsed, mazes::randomizer& rng) noexcept;

private:
    std::unique_ptr<texture> m_texture;
};

#endif // WALL_H
