#ifndef SDLHELPER_HPP
#define SDLHELPER_HPP

#include <mutex>
#include <string_view>

struct SDL_Window;
struct SDL_Renderer;

class sdl_helper
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
