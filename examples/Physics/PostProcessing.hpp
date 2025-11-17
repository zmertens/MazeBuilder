#ifndef POST_PROCESSING_HPP
#define POST_PROCESSING_HPP

#include "RenderTexture.hpp"

#include <SDL3/SDL.h>
#include <memory>
#include <vector>

/// @brief Base class for post-processing effects
class PostProcessingEffect
{
public:
    virtual ~PostProcessingEffect() = default;

    /// @brief Apply the effect to the input texture and render to output
    /// @param input Input texture
    /// @param output Output render texture
    /// @param renderer SDL renderer
    virtual void apply(SDL_Texture* input, RenderTexture& output, SDL_Renderer* renderer) = 0;

    /// @brief Check if the effect is ready to use
    [[nodiscard]] virtual bool isReady() const = 0;
};

/// @brief Box blur post-processing effect
/// @details Implements a simple box blur by averaging neighboring pixels
class BoxBlurEffect : public PostProcessingEffect
{
public:
    /// @brief Constructor
    /// @param radius Blur radius in pixels (default: 2)
    explicit BoxBlurEffect(int radius = 2);

    /// @brief Create the effect resources
    /// @param width Width of the render target
    /// @param height Height of the render target
    /// @param renderer SDL renderer
    /// @return true if successful
    bool create(int width, int height, SDL_Renderer* renderer);

    /// @brief Apply box blur effect
    void apply(SDL_Texture* input, RenderTexture& output, SDL_Renderer* renderer) override;

    /// @brief Check if the effect is ready
    [[nodiscard]] bool isReady() const override;

    /// @brief Set the blur radius
    void setRadius(int radius);

    /// @brief Get the blur radius
    [[nodiscard]] int getRadius() const noexcept { return mRadius; }

    /// @brief Set the number of blur passes (more passes = smoother blur)
    void setPasses(int passes);

    /// @brief Get the number of blur passes
    [[nodiscard]] int getPasses() const noexcept { return mPasses; }

private:
    void applyHorizontalBlur(SDL_Texture* input, RenderTexture& output, SDL_Renderer* renderer);
    void applyVerticalBlur(SDL_Texture* input, RenderTexture& output, SDL_Renderer* renderer);

    int mRadius;
    int mPasses;
    std::unique_ptr<RenderTexture> mTempTexture; // Temporary texture for multi-pass rendering
};

/// @brief Bloom post-processing effect
/// @details Extracts bright areas, blurs them, and adds them back to the original image
class BloomEffect : public PostProcessingEffect
{
public:
    /// @brief Constructor
    /// @param threshold Brightness threshold (0.0-1.0, default: 0.7)
    /// @param blurRadius Blur radius for the bloom (default: 3)
    explicit BloomEffect(float threshold = 0.7f, int blurRadius = 3);

    /// @brief Create the effect resources
    /// @param width Width of the render target
    /// @param height Height of the render target
    /// @param renderer SDL renderer
    /// @return true if successful
    bool create(int width, int height, SDL_Renderer* renderer);

    /// @brief Apply bloom effect
    void apply(SDL_Texture* input, RenderTexture& output, SDL_Renderer* renderer) override;

    /// @brief Check if the effect is ready
    [[nodiscard]] bool isReady() const override;

    /// @brief Set the brightness threshold
    void setThreshold(float threshold);

    /// @brief Get the brightness threshold
    [[nodiscard]] float getThreshold() const noexcept { return mThreshold; }

    /// @brief Set the bloom intensity
    void setIntensity(float intensity);

    /// @brief Get the bloom intensity
    [[nodiscard]] float getIntensity() const noexcept { return mIntensity; }

    /// @brief Set the blur radius
    void setBlurRadius(int radius);

    /// @brief Get the blur radius
    [[nodiscard]] int getBlurRadius() const noexcept { return mBlurRadius; }

private:
    void extractBrightPixels(SDL_Texture* input, RenderTexture& output, SDL_Renderer* renderer);
    void combineTextures(SDL_Texture* original, SDL_Texture* bloom, RenderTexture& output, SDL_Renderer* renderer);

    float mThreshold;
    float mIntensity;
    int mBlurRadius;
    std::unique_ptr<RenderTexture> mBrightTexture; // Texture for bright pixels
    std::unique_ptr<RenderTexture> mBlurTexture;   // Texture for blurred bright pixels
    std::unique_ptr<BoxBlurEffect> mBlurEffect;    // Blur effect for the bloom
};

/// @brief Post-processing pipeline that manages multiple effects
class PostProcessingPipeline
{
public:
    PostProcessingPipeline() = default;

    /// @brief Create the pipeline
    /// @param width Width of render targets
    /// @param height Height of render targets
    /// @param renderer SDL renderer
    /// @return true if successful
    bool create(int width, int height, SDL_Renderer* renderer);

    /// @brief Add an effect to the pipeline
    void addEffect(std::shared_ptr<PostProcessingEffect> effect);

    /// @brief Remove all effects
    void clearEffects();

    /// @brief Apply all effects in the pipeline
    /// @param input Input texture
    /// @param output Output render texture
    /// @param renderer SDL renderer
    void apply(SDL_Texture* input, RenderTexture& output, SDL_Renderer* renderer);

    /// @brief Check if the pipeline is ready
    [[nodiscard]] bool isReady() const;

private:
    std::vector<std::shared_ptr<PostProcessingEffect>> mEffects;
    std::unique_ptr<RenderTexture> mTempTexture1;
    std::unique_ptr<RenderTexture> mTempTexture2;
    int mWidth{0};
    int mHeight{0};
};

#endif // POST_PROCESSING_HPP

