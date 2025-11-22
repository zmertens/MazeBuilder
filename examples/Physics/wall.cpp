#include "wall.h"

#include <SDL3/SDL.h>

#include "texture.h"

#include <MazeBuilder/randomizer.h>

wall::wall(std::unique_ptr<texture> t)
    : m_texture{std::move(t)}
{
}

void wall::draw(SDL_Renderer* renderer) const noexcept
{
}

void wall::update(float elapsed, mazes::randomizer& rng) noexcept
{
}

