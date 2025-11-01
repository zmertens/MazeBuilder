#include "RenderWindow.hpp"

#include "View.hpp"

#include <SDL3/SDL.h>

RenderWindow::RenderWindow(SDL_Renderer* renderer)
    : mRenderer(renderer), mCurrentView()
{
}

void RenderWindow::setView(const View& view) {
    mCurrentView = view;
}

void RenderWindow::clear() const noexcept{
    SDL_SetRenderDrawColor(mRenderer, 240, 240, 240, 255); // Light gray background
    SDL_RenderClear(mRenderer);
}

void RenderWindow::display() const noexcept {
    SDL_RenderPresent(mRenderer);
}

bool RenderWindow::isOpen() const noexcept {
    return mRenderer != nullptr;
}