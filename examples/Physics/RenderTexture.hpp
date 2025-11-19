#ifndef RENDER_TEXTURE_HPP
#define RENDER_TEXTURE_HPP

#include "RenderStates.hpp"
#include "Texture.hpp"
#include "View.hpp"

#include <SDL3/SDL.h>
#include <memory>

/// @brief RenderTexture class for SDL3 that mimics SFML's sf::RenderTexture
/// @details This class allows rendering to a texture instead of the screen,
/// which is useful for post-processing effects like bloom, blur, etc.
class RenderTexture
{
public:
    /// @brief Default constructor
    RenderTexture();

    /// @brief Constructor with size
    /// @param width Width of the render texture in pixels
    /// @param height Height of the render texture in pixels
    /// @param renderer SDL renderer to use for creating the texture
    explicit RenderTexture(int width, int height, SDL_Renderer* renderer);

    /// @brief Destructor
    ~RenderTexture();

    // Delete copy constructor and copy assignment operator
    RenderTexture(const RenderTexture&) = delete;
    RenderTexture& operator=(const RenderTexture&) = delete;

    // Allow move semantics
    RenderTexture(RenderTexture&& other) noexcept;
    RenderTexture& operator=(RenderTexture&& other) noexcept;

    /// @brief Create or resize the render texture
    /// @param width Width of the render texture in pixels
    /// @param height Height of the render texture in pixels
    /// @param renderer SDL renderer to use for creating the texture
    /// @return true if successful, false otherwise
    bool create(int width, int height, SDL_Renderer* renderer);

    /// @brief Activate or deactivate the render texture as the current render target
    /// @param active true to activate, false to deactivate (return to default target)
    /// @return true if successful, false otherwise
    bool setActive(bool active = true);

    /// @brief Clear the render texture with a color
    /// @param r Red component (0-255)
    /// @param g Green component (0-255)
    /// @param b Blue component (0-255)
    /// @param a Alpha component (0-255)
    void clear(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255);

    /// @brief Update the contents of the target texture
    /// @details This function finalizes rendering to the texture.
    /// Call this after all drawing operations are complete.
    void display();

    /// @brief Get the underlying SDL texture
    /// @return Pointer to the SDL_Texture, or nullptr if not created
    [[nodiscard]] SDL_Texture* getTexture() const noexcept;

    /// @brief Get the width of the render texture
    [[nodiscard]] int getWidth() const noexcept { return mWidth; }

    /// @brief Get the height of the render texture
    [[nodiscard]] int getHeight() const noexcept { return mHeight; }

    /// @brief Check if the render texture is valid
    [[nodiscard]] bool isValid() const noexcept;

    /// @brief Set the view (camera) for rendering
    void setView(const View& view);

    /// @brief Get the current view
    [[nodiscard]] View getView() const noexcept;

    /// @brief Set texture smoothing (filtering)
    /// @param smooth true for linear filtering, false for nearest neighbor
    void setSmooth(bool smooth);

    /// @brief Check if texture smoothing is enabled
    [[nodiscard]] bool isSmooth() const noexcept { return mSmooth; }

    /// @brief Set blend mode for the render texture
    /// @param blendMode SDL blend mode to use
    void setBlendMode(SDL_BlendMode blendMode);

    /// @brief Get the current blend mode
    [[nodiscard]] SDL_BlendMode getBlendMode() const noexcept;

    /// @brief Draw a texture to this render texture
    /// @param texture Texture to draw
    /// @param srcRect Source rectangle (nullptr for entire texture)
    /// @param dstRect Destination rectangle (nullptr for entire render target)
    void draw(SDL_Texture* texture, const SDL_FRect* srcRect = nullptr, const SDL_FRect* dstRect = nullptr);

    /// @brief Draw a drawable object to this render texture
    /// @tparam T Any type with a draw(SDL_Renderer*, RenderStates) method
    template <typename T>
    void draw(const T& drawable);

    /// @brief Draw a drawable object with custom render states
    /// @tparam T Any type with a draw(SDL_Renderer*, RenderStates) method
    template <typename T>
    void draw(const T& drawable, const RenderStates& states);

    /// @brief Get the SDL renderer associated with this render texture
    [[nodiscard]] SDL_Renderer* getRenderer() const noexcept { return mRenderer; }

private:
    SDL_Texture* mTexture;      ///< The target texture
    SDL_Renderer* mRenderer;    ///< The SDL renderer
    SDL_Texture* mPrevTarget;   ///< Previous render target (for restoration)
    int mWidth;                 ///< Width of the render texture
    int mHeight;                ///< Height of the render texture
    bool mSmooth;               ///< Texture smoothing enabled
    SDL_BlendMode mBlendMode;   ///< Current blend mode
    View mCurrentView;          ///< Current view/camera
    bool mActive;               ///< Whether this render texture is currently active
};

// Template implementations
template <typename T>
void RenderTexture::draw(const T& drawable)
{
    RenderStates states;
    draw(drawable, states);
}

template <typename T>
void RenderTexture::draw(const T& drawable, const RenderStates& states)
{
    if (!isValid() || !mActive)
    {
        SDL_Log("ERROR: Cannot draw to inactive or invalid RenderTexture");
        return;
    }

    drawable.draw(mRenderer, states);
}

#endif // RENDER_TEXTURE_HPP

