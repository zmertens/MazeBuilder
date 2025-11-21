#ifndef RENDER_WINDOW_HPP
#define RENDER_WINDOW_HPP

struct SDL_Renderer;
struct SDL_Window;
class View;

/// @brief SDL-based RenderWindow that mimics SFML's sf::RenderWindow interface
class render_window
{
public:
    explicit render_window(SDL_Renderer* renderer, SDL_Window* window);

    virtual ~render_window() = default;

    // Delete copy constructor and copy assignment operator
    // because RenderWindow contains std::unique_ptr which is not copyable
    render_window(const render_window&) = delete;
    render_window& operator=(const render_window&) = delete;

    // Allow move constructor and move assignment operator
    render_window(render_window&&) = default;
    render_window& operator=(render_window&&) = default;

    /// @brief Draw a drawable object (like RenderWindow)
    /// @tparam T Any type with a draw method
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
};

template <typename T>
void render_window::draw(const T& drawable) const noexcept
{
    drawable.draw(mRenderer);
}

#endif // RENDER_WINDOW_HPP
