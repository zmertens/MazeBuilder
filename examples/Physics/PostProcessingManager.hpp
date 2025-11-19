#ifndef POST_PROCESSING_MANAGER_HPP
#define POST_PROCESSING_MANAGER_HPP

#include "RenderTexture.hpp"
#include "PostProcessing.hpp"

#include <SDL3/SDL.h>
#include <memory>
#include <string>

/// @brief Manager class for post-processing effects in the game
/// @details This class manages render textures and effects,
/// making it easy to integrate post-processing into existing rendering code
class PostProcessingManager
{
public:
    PostProcessingManager() = default;

    /// @brief Initialize the post-processing manager
    /// @param renderer SDL renderer
    /// @param width Render target width
    /// @param height Render target height
    /// @return true if successful
    bool initialize(SDL_Renderer* renderer, int width, int height);

    /// @brief Shutdown and cleanup
    void shutdown();

    /// @brief Begin rendering to the scene texture
    /// @return true if successful
    bool beginScene();

    /// @brief End scene rendering
    void endScene();

    /// @brief Apply all enabled effects and render final result
    /// @param targetRenderer Renderer to draw final result to (usually screen renderer)
    void present(SDL_Renderer* targetRenderer);

    /// @brief Toggle effects
    void enableBoxBlur(bool enable) { mBoxBlurEnabled = enable; }
    void enableBloom(bool enable) { mBloomEnabled = enable; }

    /// @brief Check effect status
    [[nodiscard]] bool isBoxBlurEnabled() const { return mBoxBlurEnabled; }
    [[nodiscard]] bool isBloomEnabled() const { return mBloomEnabled; }

    /// @brief Configure box blur
    void setBlurRadius(int radius);
    void setBlurPasses(int passes);

    /// @brief Configure bloom
    void setBloomThreshold(float threshold);
    void setBloomIntensity(float intensity);
    void setBloomBlurRadius(int radius);

    /// @brief Get the scene renderer (for drawing)
    /// @return Renderer to use for scene drawing
    [[nodiscard]] SDL_Renderer* getSceneRenderer() const;

    /// @brief Check if post-processing is ready
    [[nodiscard]] bool isReady() const;

    /// @brief Resize render targets
    bool resize(int width, int height);

private:
    void applyEffects();

    SDL_Renderer* mRenderer{nullptr};
    std::unique_ptr<RenderTexture> mSceneTexture;
    std::unique_ptr<RenderTexture> mFinalTexture;

    std::shared_ptr<BoxBlurEffect> mBoxBlur;
    std::shared_ptr<BloomEffect> mBloom;
    std::unique_ptr<PostProcessingPipeline> mPipeline;

    bool mBoxBlurEnabled{false};
    bool mBloomEnabled{false};
    bool mSceneActive{false};

    int mWidth{0};
    int mHeight{0};
};

#endif // POST_PROCESSING_MANAGER_HPP

