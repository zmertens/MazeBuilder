#include "RenderWindow.hpp"

#include "View.hpp"

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_sdlrenderer3.h>

#include <SDL3/SDL.h>

RenderWindow::RenderWindow(SDL_Renderer* renderer, SDL_Window* window)
    : mRenderer(renderer), mWindow(window), mCurrentView()
{
}

void RenderWindow::setView(const View& view)
{
    mCurrentView = view;
}

View RenderWindow::getView() const noexcept
{
    return mCurrentView;
}

void RenderWindow::clear() const noexcept
{
    if (!mRenderer || !mWindow)
    {
        return;
    }
    SDL_RenderClear(mRenderer);
}

void RenderWindow::display() const noexcept
{
    if (!mRenderer || !mWindow)
    {
        return;
    }

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();

    SDL_RenderPresent(mRenderer);

    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mRenderer);
}

bool RenderWindow::isOpen() const noexcept
{
    return mRenderer != nullptr && mWindow != nullptr;
}

void RenderWindow::close() noexcept
{
    // Just null out the pointers to signal the window is closed
    // Don't destroy the actual SDL resources - that's SDLHelper's job
    // during proper cleanup in its destructor
    SDL_Log("RenderWindow::close() - Marking window as closed\n");

    mRenderer = nullptr;
    mWindow = nullptr;
}
