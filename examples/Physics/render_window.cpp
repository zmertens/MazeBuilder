#include "render_window.h"

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_sdlrenderer3.h>

#include <SDL3/SDL.h>

render_window::render_window(SDL_Renderer* renderer, SDL_Window* window)
    : mRenderer(renderer), mWindow(window)
{
    // Initialize view with window dimensions
    if (mWindow)
    {
        int width = 0, height = 0;
        SDL_GetWindowSize(mWindow, &width, &height);
    }
}

void render_window::beginFrame() const noexcept
{
    if (!mRenderer || !mWindow)
    {
        return;
    }

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();
}

void render_window::clear() const noexcept
{
    if (!mRenderer || !mWindow)
    {
        return;
    }
    SDL_RenderClear(mRenderer);
}

void render_window::display() const noexcept
{
    if (!mRenderer || !mWindow)
    {
        return;
    }

    ImGui::Render();
    SDL_SetRenderScale(mRenderer, ImGui::GetIO().DisplayFramebufferScale.x, ImGui::GetIO().DisplayFramebufferScale.y);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mRenderer);
    SDL_RenderPresent(mRenderer);
}

bool render_window::isOpen() const noexcept
{
    return mRenderer != nullptr && mWindow != nullptr;
}

void render_window::close() noexcept
{
    // Just null out the pointers to signal the window is closed
    // Don't destroy the actual SDL resources - that's SDLHelper's job
    // during proper cleanup in its destructor
    SDL_Log("RenderWindow::close() - Marking window as closed\n");

    mRenderer = nullptr;
    mWindow = nullptr;
}

void render_window::setFullscreen(bool fullscreen) const noexcept
{
    if (const auto flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0; !isFullscreen() && flags != 0)
    {
        SDL_SetWindowFullscreen(mWindow, flags);
    }
    else if (isFullscreen() && !fullscreen)
    {
        SDL_SetWindowFullscreen(mWindow, false);
    }
}

bool render_window::isFullscreen() const noexcept
{
    const auto flags = SDL_GetWindowFlags(mWindow);
    return (flags & SDL_WINDOW_FULLSCREEN) != 0;
}
