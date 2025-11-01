#ifndef RENDER_WINDOW_HPP
#define RENDER_WINDOW_HPP

#include "RenderStates.hpp"
#include "View.hpp"

struct SDL_Renderer;

/// @brief SDL-based RenderWindow that mimics SFML's sf::RenderWindow interface
class RenderWindow {
public:
    explicit RenderWindow(SDL_Renderer* renderer);

    /// @brief Set the current view (camera) for rendering
    void setView(const View& view);

    /// @brief Draw a drawable object (like SceneNode)
    /// @tparam T Any type with a draw(RenderStates) method
    template<typename T>
    void draw(const T& drawable);

    /// @brief Clear the render target
    void clear();

    /// @brief Present the rendered frame
    void display();

private:
    SDL_Renderer* mRenderer;
    View mCurrentView;
};

// Template implementation
template<typename T>
void RenderWindow::draw(const T& drawable) {
    RenderStates states;
    // Apply view transform if needed (for camera/scrolling)
    // states.transform could be modified based on mCurrentView here
    drawable.draw(states);
}

#endif // RENDER_WINDOW_HPP
