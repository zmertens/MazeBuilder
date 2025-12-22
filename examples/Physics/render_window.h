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

    void begin_frame() const noexcept;

    /// @brief Clear the render target
    void clear() const noexcept;

    /// @brief Present the rendered frame
    void display() const noexcept;

    [[nodiscard]] bool is_open() const noexcept;

    void close() noexcept;

    void set_fullscreen(bool fullscreen) const noexcept;

    [[nodiscard]] bool is_fullscreen() const noexcept;

    /// @brief Get the SDL renderer for direct access
    [[nodiscard]] SDL_Renderer* get_renderer() const noexcept;

    /// @brief Get the SDL window for direct access
    [[nodiscard]] SDL_Window* get_window() const noexcept;

private:
    SDL_Renderer* m_renderer;
    SDL_Window* m_window;
};

template <typename T>
void render_window::draw(const T& drawable) const noexcept
{
    drawable.draw(m_renderer);
}

#endif // RENDER_WINDOW_HPP
