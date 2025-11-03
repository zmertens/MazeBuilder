#include "RenderWindow.hpp"

#include <MazeBuilder/singleton_base.h>

#include "SDLHelper.hpp"
#include "View.hpp"

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
    if (!mRenderer)
    {
        return; // Window is closed, skip
    }
    SDL_RenderClear(mRenderer);
}

void RenderWindow::display() const noexcept
{
    if (!mRenderer)
    {
        return; // Window is closed, skip
    }
    SDL_RenderPresent(mRenderer);
}

bool RenderWindow::isOpen() const noexcept
{
    return mRenderer != nullptr;
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
