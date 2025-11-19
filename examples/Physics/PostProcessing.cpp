#include "PostProcessing.hpp"

#include <algorithm>
#include <cmath>

// ========================================
// BoxBlurEffect Implementation
// ========================================

BoxBlurEffect::BoxBlurEffect(int radius)
    : mRadius(std::max(1, radius))
    , mPasses(1)
    , mTempTexture(nullptr)
{
}

bool BoxBlurEffect::create(int width, int height, SDL_Renderer* renderer)
{
    if (!renderer || width <= 0 || height <= 0)
    {
        SDL_Log("ERROR: BoxBlurEffect::create - Invalid parameters");
        return false;
    }

    mTempTexture = std::make_unique<RenderTexture>(width, height, renderer);
    if (!mTempTexture->isValid())
    {
        SDL_Log("ERROR: BoxBlurEffect::create - Failed to create temporary texture");
        return false;
    }

    return true;
}

void BoxBlurEffect::apply(SDL_Texture* input, RenderTexture& output, SDL_Renderer* renderer)
{
    if (!isReady() || !input || !renderer)
    {
        SDL_Log("ERROR: BoxBlurEffect::apply - Invalid state or parameters");
        return;
    }

    // For a simple box blur, we do horizontal then vertical passes
    // For better quality, we can do multiple passes

    for (int pass = 0; pass < mPasses; ++pass)
    {
        // Horizontal blur pass
        if (pass == 0)
        {
            applyHorizontalBlur(input, *mTempTexture, renderer);
        }
        else
        {
            applyHorizontalBlur(output.getTexture(), *mTempTexture, renderer);
        }

        // Vertical blur pass
        if (pass == mPasses - 1)
        {
            applyVerticalBlur(mTempTexture->getTexture(), output, renderer);
        }
        else
        {
            applyVerticalBlur(mTempTexture->getTexture(), output, renderer);
        }
    }
}

bool BoxBlurEffect::isReady() const
{
    return mTempTexture && mTempTexture->isValid();
}

void BoxBlurEffect::setRadius(int radius)
{
    mRadius = std::max(1, radius);
}

void BoxBlurEffect::setPasses(int passes)
{
    mPasses = std::max(1, passes);
}

void BoxBlurEffect::applyHorizontalBlur(SDL_Texture* input, RenderTexture& output, SDL_Renderer* renderer)
{
    if (!output.setActive(true))
    {
        SDL_Log("ERROR: BoxBlurEffect::applyHorizontalBlur - Failed to activate render texture");
        return;
    }

    output.clear(0, 0, 0, 0);

    // NOTE: SDL3 doesn't have built-in shader support like OpenGL/SFML
    // For a true box blur, you would need to:
    // 1. Use SDL_GPU for shader-based effects, or
    // 2. Manually read pixels, blur them in CPU, and write back (very slow), or
    // 3. Use a third-party library like SDL_gpu or implement custom rendering

    // For this simple implementation, we'll just render the texture with reduced opacity
    // This simulates a very basic blur effect
    // In a real implementation, you'd want to use shaders for proper blur

    float alpha = 1.0f / (mRadius * 2 + 1);
    SDL_SetTextureAlphaModFloat(input, alpha);

    // Render multiple offset copies
    for (int i = -mRadius; i <= mRadius; ++i)
    {
        SDL_FRect destRect;
        destRect.x = static_cast<float>(i);
        destRect.y = 0.0f;
        destRect.w = static_cast<float>(output.getWidth());
        destRect.h = static_cast<float>(output.getHeight());

        output.draw(input, nullptr, &destRect);
    }

    SDL_SetTextureAlphaModFloat(input, 1.0f); // Reset alpha

    output.display();
    output.setActive(false);
}

void BoxBlurEffect::applyVerticalBlur(SDL_Texture* input, RenderTexture& output, SDL_Renderer* renderer)
{
    if (!output.setActive(true))
    {
        SDL_Log("ERROR: BoxBlurEffect::applyVerticalBlur - Failed to activate render texture");
        return;
    }

    output.clear(0, 0, 0, 0);

    float alpha = 1.0f / (mRadius * 2 + 1);
    SDL_SetTextureAlphaModFloat(input, alpha);

    // Render multiple offset copies
    for (int i = -mRadius; i <= mRadius; ++i)
    {
        SDL_FRect destRect;
        destRect.x = 0.0f;
        destRect.y = static_cast<float>(i);
        destRect.w = static_cast<float>(output.getWidth());
        destRect.h = static_cast<float>(output.getHeight());

        output.draw(input, nullptr, &destRect);
    }

    SDL_SetTextureAlphaModFloat(input, 1.0f); // Reset alpha

    output.display();
    output.setActive(false);
}

// ========================================
// BloomEffect Implementation
// ========================================

BloomEffect::BloomEffect(float threshold, int blurRadius)
    : mThreshold(std::clamp(threshold, 0.0f, 1.0f))
    , mIntensity(1.0f)
    , mBlurRadius(std::max(1, blurRadius))
    , mBrightTexture(nullptr)
    , mBlurTexture(nullptr)
    , mBlurEffect(nullptr)
{
}

bool BloomEffect::create(int width, int height, SDL_Renderer* renderer)
{
    if (!renderer || width <= 0 || height <= 0)
    {
        SDL_Log("ERROR: BloomEffect::create - Invalid parameters");
        return false;
    }

    // Create textures for bloom pipeline
    mBrightTexture = std::make_unique<RenderTexture>(width, height, renderer);
    if (!mBrightTexture->isValid())
    {
        SDL_Log("ERROR: BloomEffect::create - Failed to create bright texture");
        return false;
    }

    mBlurTexture = std::make_unique<RenderTexture>(width, height, renderer);
    if (!mBlurTexture->isValid())
    {
        SDL_Log("ERROR: BloomEffect::create - Failed to create blur texture");
        return false;
    }

    // Create blur effect for the bloom
    mBlurEffect = std::make_unique<BoxBlurEffect>(mBlurRadius);
    if (!mBlurEffect->create(width, height, renderer))
    {
        SDL_Log("ERROR: BloomEffect::create - Failed to create blur effect");
        return false;
    }

    return true;
}

void BloomEffect::apply(SDL_Texture* input, RenderTexture& output, SDL_Renderer* renderer)
{
    if (!isReady() || !input || !renderer)
    {
        SDL_Log("ERROR: BloomEffect::apply - Invalid state or parameters");
        return;
    }

    // Step 1: Extract bright pixels
    extractBrightPixels(input, *mBrightTexture, renderer);

    // Step 2: Blur the bright pixels
    mBlurEffect->apply(mBrightTexture->getTexture(), *mBlurTexture, renderer);

    // Step 3: Combine original with blurred bloom
    combineTextures(input, mBlurTexture->getTexture(), output, renderer);
}

bool BloomEffect::isReady() const
{
    return mBrightTexture && mBrightTexture->isValid() &&
           mBlurTexture && mBlurTexture->isValid() &&
           mBlurEffect && mBlurEffect->isReady();
}

void BloomEffect::setThreshold(float threshold)
{
    mThreshold = std::clamp(threshold, 0.0f, 1.0f);
}

void BloomEffect::setIntensity(float intensity)
{
    mIntensity = std::max(0.0f, intensity);
}

void BloomEffect::setBlurRadius(int radius)
{
    mBlurRadius = std::max(1, radius);
    if (mBlurEffect)
    {
        mBlurEffect->setRadius(mBlurRadius);
    }
}

void BloomEffect::extractBrightPixels(SDL_Texture* input, RenderTexture& output, SDL_Renderer* renderer)
{
    if (!output.setActive(true))
    {
        SDL_Log("ERROR: BloomEffect::extractBrightPixels - Failed to activate render texture");
        return;
    }

    output.clear(0, 0, 0, 0);

    // NOTE: For a proper bright pixel extraction, you would need shader support
    // This is a simplified version that just renders the input with modified alpha
    // based on the threshold

    // In a real implementation with shaders, you would:
    // 1. Calculate luminance for each pixel
    // 2. Compare with threshold
    // 3. Output bright pixels only

    // For now, we'll just render the input directly and rely on blend modes
    SDL_SetTextureBlendMode(input, SDL_BLENDMODE_BLEND);

    output.draw(input, nullptr, nullptr);

    output.display();
    output.setActive(false);
}

void BloomEffect::combineTextures(SDL_Texture* original, SDL_Texture* bloom, RenderTexture& output, SDL_Renderer* renderer)
{
    if (!output.setActive(true))
    {
        SDL_Log("ERROR: BloomEffect::combineTextures - Failed to activate render texture");
        return;
    }

    output.clear(0, 0, 0, 255);

    // First, render the original image
    output.draw(original, nullptr, nullptr);

    // Then, add the bloom on top with additive blending
    SDL_SetTextureBlendMode(bloom, SDL_BLENDMODE_ADD);
    SDL_SetTextureAlphaModFloat(bloom, mIntensity);

    output.draw(bloom, nullptr, nullptr);

    // Reset blend mode and alpha
    SDL_SetTextureBlendMode(bloom, SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaModFloat(bloom, 1.0f);

    output.display();
    output.setActive(false);
}

// ========================================
// PostProcessingPipeline Implementation
// ========================================

bool PostProcessingPipeline::create(int width, int height, SDL_Renderer* renderer)
{
    if (!renderer || width <= 0 || height <= 0)
    {
        SDL_Log("ERROR: PostProcessingPipeline::create - Invalid parameters");
        return false;
    }

    mWidth = width;
    mHeight = height;

    // Create temporary textures for ping-pong rendering
    mTempTexture1 = std::make_unique<RenderTexture>(width, height, renderer);
    if (!mTempTexture1->isValid())
    {
        SDL_Log("ERROR: PostProcessingPipeline::create - Failed to create temp texture 1");
        return false;
    }

    mTempTexture2 = std::make_unique<RenderTexture>(width, height, renderer);
    if (!mTempTexture2->isValid())
    {
        SDL_Log("ERROR: PostProcessingPipeline::create - Failed to create temp texture 2");
        return false;
    }

    return true;
}

void PostProcessingPipeline::addEffect(std::shared_ptr<PostProcessingEffect> effect)
{
    if (effect && effect->isReady())
    {
        mEffects.push_back(effect);
    }
    else
    {
        SDL_Log("WARNING: PostProcessingPipeline::addEffect - Effect is not ready");
    }
}

void PostProcessingPipeline::clearEffects()
{
    mEffects.clear();
}

void PostProcessingPipeline::apply(SDL_Texture* input, RenderTexture& output, SDL_Renderer* renderer)
{
    if (!isReady() || !input || !renderer || mEffects.empty())
    {
        // No effects, just copy input to output
        if (output.setActive(true))
        {
            output.clear(0, 0, 0, 255);
            output.draw(input, nullptr, nullptr);
            output.display();
            output.setActive(false);
        }
        return;
    }

    // Apply effects in sequence using ping-pong rendering
    SDL_Texture* currentInput = input;

    for (size_t i = 0; i < mEffects.size(); ++i)
    {
        if (i == mEffects.size() - 1)
        {
            // Last effect renders to final output
            mEffects[i]->apply(currentInput, output, renderer);
        }
        else
        {
            // Intermediate effects render to temporary textures
            RenderTexture& tempOutput = (i % 2 == 0) ? *mTempTexture1 : *mTempTexture2;
            mEffects[i]->apply(currentInput, tempOutput, renderer);
            currentInput = tempOutput.getTexture();
        }
    }
}

bool PostProcessingPipeline::isReady() const
{
    return mTempTexture1 && mTempTexture1->isValid() &&
           mTempTexture2 && mTempTexture2->isValid();
}

