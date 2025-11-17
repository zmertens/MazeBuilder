#ifndef RENDER_WINDOW_HPP
#define RENDER_WINDOW_HPP

#include "RenderStates.hpp"
#include "View.hpp"

struct SDL_Renderer;
struct SDL_Window;
class View;

/// @brief SDL-based RenderWindow that mimics SFML's sf::RenderWindow interface
class RenderWindow
{
public:
    explicit RenderWindow(SDL_Renderer* renderer, SDL_Window* window);

    virtual ~RenderWindow() = default;

    // Delete copy constructor and copy assignment operator
    // because RenderWindow contains std::unique_ptr which is not copyable
    RenderWindow(const RenderWindow&) = delete;
    RenderWindow& operator=(const RenderWindow&) = delete;

    // Allow move constructor and move assignment operator
    RenderWindow(RenderWindow&&) = default;
    RenderWindow& operator=(RenderWindow&&) = default;

    /// @brief Set the current view (camera) for rendering
    void setView(const View& view);

    [[nodiscard]] View getView() const noexcept;

    /// @brief Draw a drawable object (like RenderWindow)
    /// @tparam T Any type with a draw(RenderStates) method
    template <typename T>
    void draw(const T& drawable) const noexcept;

    void beginFrame() const noexcept;

    /// @brief Clear the render target
    void clear() const noexcept;

    /// @brief Present the rendered frame
    void display() const noexcept;

    [[nodiscard]] bool isOpen() const noexcept;

    void close() noexcept;

    void setFullscreen(bool fullscreen) const noexcept;

    [[nodiscard]] bool isFullscreen() const noexcept;

    /// @brief Get the SDL renderer for direct access
    [[nodiscard]] SDL_Renderer* getRenderer() const noexcept { return mRenderer; }

    /// @brief Get the SDL window for direct access
    [[nodiscard]] SDL_Window* getSDLWindow() const noexcept { return mWindow; }

private:
    SDL_Renderer* mRenderer;
    SDL_Window* mWindow;
    View mCurrentView;
};

template <typename T>
void RenderWindow::draw(const T& drawable) const noexcept
{
    RenderStates states;
    // Apply view transform if needed (for camera/scrolling)
    // states.transform could be modified based on mCurrentView here
    drawable.draw(mRenderer, states);
}

#endif // RENDER_WINDOW_HPP
