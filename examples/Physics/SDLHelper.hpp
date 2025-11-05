#ifndef SDLHELPER_HPP
#define SDLHELPER_HPP

#include <mutex>
#include <string_view>

#include "State.hpp"

struct SDL_Window;
struct SDL_Renderer;

class SDLHelper
{
public:
    SDL_Window* window;

    SDL_Renderer* renderer;

private:
    std::once_flag sdlInitializedFlag;

public:
    void init(std::string_view title, int width, int height) noexcept;

    void destroyAndQuit() noexcept;
}; // SDLHelper class

#endif // SDLHELPER_HPP
