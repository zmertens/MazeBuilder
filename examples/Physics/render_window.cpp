#include "render_window.h"

#include <dearimgui/imgui.h>
#include <dearimgui/backends/imgui_impl_sdl3.h>
#include <dearimgui/backends/imgui_impl_sdlrenderer3.h>

#include <SDL3/SDL.h>

render_window::render_window(SDL_Renderer* renderer, SDL_Window* window)
    : m_renderer(renderer), m_window(window)
{
    // Initialize view with window dimensions
    if (m_window)
    {
        int width = 0, height = 0;
        SDL_GetWindowSize(m_window, &width, &height);
    }
}

void render_window::begin_frame() const noexcept
{
    if (!m_renderer || !m_window)
    {
        return;
    }

    ImGui_ImplSDL3_NewFrame();
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui::NewFrame();
}

void render_window::clear() const noexcept
{
    if (!m_renderer || !m_window)
    {
        return;
    }
    SDL_RenderClear(m_renderer);
}

void render_window::display() const noexcept
{
    if (!m_renderer || !m_window)
    {
        return;
    }

    ImGui::Render();
    SDL_SetRenderScale(m_renderer, ImGui::GetIO().DisplayFramebufferScale.x, ImGui::GetIO().DisplayFramebufferScale.y);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
    SDL_RenderPresent(m_renderer);
}

bool render_window::is_open() const noexcept
{
    return m_renderer != nullptr && m_window != nullptr;
}

void render_window::close() noexcept
{
    // Just null out the pointers to signal the window is closed
    // Don't destroy the actual SDL resources - that's SDLHelper's job
    // during proper cleanup in its destructor
    SDL_Log("RenderWindow::close() - Marking window as closed\n");

    m_renderer = nullptr;
    m_window = nullptr;
}

void render_window::set_fullscreen(bool fullscreen) const noexcept
{
    if (const auto flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0; !is_fullscreen() && flags != 0)
    {
        SDL_SetWindowFullscreen(m_window, flags);
    }
    else if (is_fullscreen() && !fullscreen)
    {
        SDL_SetWindowFullscreen(m_window, false);
    }
}

bool render_window::is_fullscreen() const noexcept
{
    const auto flags = SDL_GetWindowFlags(m_window);
    return (flags & SDL_WINDOW_FULLSCREEN) != 0;
}

SDL_Renderer* render_window::get_renderer() const noexcept
{
    return m_renderer;
}

/// @brief Get the SDL window for direct access
SDL_Window* render_window::get_window() const noexcept
{
    return m_window;
}
