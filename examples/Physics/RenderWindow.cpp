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
    SDL_SetRenderDrawColor(mRenderer, 0, 128, 255, 255); // Bright blue for testing
    SDL_RenderClear(mRenderer);
    SDL_Log("RenderWindow::clear - Cleared to blue");
}

void RenderWindow::display() const noexcept {
    // Draw a test rectangle to verify rendering works
    SDL_SetRenderDrawColor(mRenderer, 255, 0, 0, 255); // Red
    SDL_FRect testRect = {100, 100, 200, 200};
    SDL_RenderFillRect(mRenderer, &testRect);
    
    SDL_Log("RenderWindow::display - Presenting frame");
    SDL_RenderPresent(mRenderer);
}

bool RenderWindow::isOpen() const noexcept {
    return mRenderer != nullptr;
}