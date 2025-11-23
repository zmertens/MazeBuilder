#ifndef SDL_HELPER_H
#define SDL_HELPER_H

#include <mutex>
#include <string_view>

struct SDL_Window;
struct SDL_Renderer;

class sdl_helper
{
public:
    SDL_Window* window;

    SDL_Renderer* renderer;

    void init(std::string_view title, int width, int height) noexcept;

    void destroy_and_quit() noexcept;

private:
    std::once_flag sdl_initialized_flag;
};

#endif // SDL_HELPER_H
