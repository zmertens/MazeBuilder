#include "SDLHelper.hpp"

#include <SDL3/SDL.h>

void SDLHelper::init(std::string_view title, int width, int height) noexcept
{
    auto initFunc = [this, title, width, height]()
    {
        this->window = SDL_CreateWindow(title.data(), width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS);

        if (!this->window)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateWindow failed: %s\n", SDL_GetError());

            return;
        }

        this->renderer = SDL_CreateRenderer(this->window, nullptr);

        if (!this->renderer)
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
            SDL_DestroyWindow(this->window);

            return;
        }

        if (auto props = SDL_GetRendererProperties(this->renderer); props != 0)
        {
            SDL_Log("Renderer created: %s\n", SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, "default"));
        }
        else
        {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to get renderer info: %s\n", SDL_GetError());

            return;
        }

        SDL_SetRenderVSync(renderer, 1);

        // Verify renderer is ready
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        SDL_Log("SDLHelper::init - Test render complete");
    };

    // SDL_Init returns true on SUCCESS (SDL3 behavior)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::call_once(sdlInitializedFlag, initFunc);
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "SDL_Init failed: %s\n", SDL_GetError());
    }
}

void SDLHelper::destroyAndQuit() noexcept
{
    // Prevent double-destruction
    if (!this->window && !this->renderer)
    {
        return;
    }

    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
#if defined(MAZE_DEBUG)

        SDL_Log("SDLHelper::destroy() - Destroying renderer %p\n", static_cast<void*>(renderer));
#endif // MAZE_DEBUG
        renderer = nullptr;
    }

    if (window)
    {
        SDL_DestroyWindow(window);
#if defined(MAZE_DEBUG)

        SDL_Log("SDLHelper::destroy() - Destroying window %p\n", static_cast<void*>(window));
#endif // MAZE_DEBUG
        window = nullptr;
    }

    SDL_Quit();
}
