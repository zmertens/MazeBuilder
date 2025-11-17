#include "RenderTexture.hpp"

#include <SDL3/SDL.h>

RenderTexture::RenderTexture()
    : mTexture(nullptr)
    , mRenderer(nullptr)
    , mPrevTarget(nullptr)
    , mWidth(0)
    , mHeight(0)
    , mSmooth(false)
    , mBlendMode(SDL_BLENDMODE_BLEND)
    , mCurrentView()
    , mActive(false)
{
}

RenderTexture::RenderTexture(int width, int height, SDL_Renderer* renderer)
    : RenderTexture()
{
    create(width, height, renderer);
}

RenderTexture::~RenderTexture()
{
    if (mActive)
    {
        setActive(false);
    }

    if (mTexture)
    {
        SDL_DestroyTexture(mTexture);
        mTexture = nullptr;
    }
}

RenderTexture::RenderTexture(RenderTexture&& other) noexcept
    : mTexture(other.mTexture)
    , mRenderer(other.mRenderer)
    , mPrevTarget(other.mPrevTarget)
    , mWidth(other.mWidth)
    , mHeight(other.mHeight)
    , mSmooth(other.mSmooth)
    , mBlendMode(other.mBlendMode)
    , mCurrentView(other.mCurrentView)
    , mActive(other.mActive)
{
    other.mTexture = nullptr;
    other.mRenderer = nullptr;
    other.mPrevTarget = nullptr;
    other.mWidth = 0;
    other.mHeight = 0;
    other.mActive = false;
}

RenderTexture& RenderTexture::operator=(RenderTexture&& other) noexcept
{
    if (this != &other)
    {
        // Clean up existing resources
        if (mActive)
        {
            setActive(false);
        }
        if (mTexture)
        {
            SDL_DestroyTexture(mTexture);
        }

        // Move from other
        mTexture = other.mTexture;
        mRenderer = other.mRenderer;
        mPrevTarget = other.mPrevTarget;
        mWidth = other.mWidth;
        mHeight = other.mHeight;
        mSmooth = other.mSmooth;
        mBlendMode = other.mBlendMode;
        mCurrentView = other.mCurrentView;
        mActive = other.mActive;

        // Reset other
        other.mTexture = nullptr;
        other.mRenderer = nullptr;
        other.mPrevTarget = nullptr;
        other.mWidth = 0;
        other.mHeight = 0;
        other.mActive = false;
    }
    return *this;
}

bool RenderTexture::create(int width, int height, SDL_Renderer* renderer)
{
    if (!renderer)
    {
        SDL_Log("ERROR: RenderTexture::create - Invalid renderer");
        return false;
    }

    if (width <= 0 || height <= 0)
    {
        SDL_Log("ERROR: RenderTexture::create - Invalid dimensions: %dx%d", width, height);
        return false;
    }

    // Clean up existing texture if any
    if (mTexture)
    {
        if (mActive)
        {
            setActive(false);
        }
        SDL_DestroyTexture(mTexture);
        mTexture = nullptr;
    }

    mRenderer = renderer;
    mWidth = width;
    mHeight = height;

    // Create texture with target access
    mTexture = SDL_CreateTexture(
        mRenderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        mWidth,
        mHeight
    );

    if (!mTexture)
    {
        SDL_Log("ERROR: RenderTexture::create - Failed to create texture: %s", SDL_GetError());
        return false;
    }

    // Set default blend mode
    if (!SDL_SetTextureBlendMode(mTexture, mBlendMode))
    {
        SDL_Log("WARNING: RenderTexture::create - Failed to set blend mode: %s", SDL_GetError());
    }

    // Set scale mode based on smooth setting
    SDL_ScaleMode scaleMode = mSmooth ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST;
    if (!SDL_SetTextureScaleMode(mTexture, scaleMode))
    {
        SDL_Log("WARNING: RenderTexture::create - Failed to set scale mode: %s", SDL_GetError());
    }

    SDL_Log("RenderTexture::create - Created %dx%d texture successfully", mWidth, mHeight);
    return true;
}

bool RenderTexture::setActive(bool active)
{
    if (!mRenderer)
    {
        SDL_Log("ERROR: RenderTexture::setActive - Invalid renderer");
        return false;
    }

    if (active)
    {
        if (!mTexture)
        {
            SDL_Log("ERROR: RenderTexture::setActive - No texture created");
            return false;
        }

        // Store the previous render target
        mPrevTarget = SDL_GetRenderTarget(mRenderer);

        // Set this texture as the render target
        if (!SDL_SetRenderTarget(mRenderer, mTexture))
        {
            SDL_Log("ERROR: RenderTexture::setActive - Failed to set render target: %s", SDL_GetError());
            return false;
        }

        mActive = true;
    }
    else
    {
        // Restore the previous render target
        if (!SDL_SetRenderTarget(mRenderer, mPrevTarget))
        {
            SDL_Log("ERROR: RenderTexture::setActive - Failed to restore render target: %s", SDL_GetError());
            return false;
        }

        mPrevTarget = nullptr;
        mActive = false;
    }

    return true;
}

void RenderTexture::clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (!isValid() || !mActive)
    {
        SDL_Log("WARNING: RenderTexture::clear - Render texture is not active");
        return;
    }

    SDL_SetRenderDrawColor(mRenderer, r, g, b, a);
    SDL_RenderClear(mRenderer);
}

void RenderTexture::display()
{
    // In SDL3, we don't need to explicitly "finalize" the render target
    // But we can optionally deactivate it here
    // For now, we just ensure rendering is flushed
    if (mActive && mRenderer)
    {
        SDL_FlushRenderer(mRenderer);
    }
}

SDL_Texture* RenderTexture::getTexture() const noexcept
{
    return mTexture;
}

bool RenderTexture::isValid() const noexcept
{
    return mTexture != nullptr && mRenderer != nullptr;
}

void RenderTexture::setView(const View& view)
{
    mCurrentView = view;

    if (!isValid() || !mActive)
    {
        return;
    }

    // Apply view transformations to the renderer
    // This is a simplified implementation - you may need to expand based on your needs
    SDL_FPoint center = view.getCenter();
    SDL_FPoint size = view.getSize();

    // Calculate viewport based on view
    SDL_FRect viewport;
    viewport.x = center.x - size.x / 2.0f;
    viewport.y = center.y - size.y / 2.0f;
    viewport.w = size.x;
    viewport.h = size.y;

    // Note: SDL3 doesn't have a direct viewport concept like SFML
    // You may need to implement view transformations in your rendering code
}

View RenderTexture::getView() const noexcept
{
    return mCurrentView;
}

void RenderTexture::setSmooth(bool smooth)
{
    mSmooth = smooth;

    if (mTexture)
    {
        SDL_ScaleMode scaleMode = mSmooth ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST;
        if (!SDL_SetTextureScaleMode(mTexture, scaleMode))
        {
            SDL_Log("WARNING: RenderTexture::setSmooth - Failed to set scale mode: %s", SDL_GetError());
        }
    }
}

void RenderTexture::setBlendMode(SDL_BlendMode blendMode)
{
    mBlendMode = blendMode;

    if (mTexture)
    {
        if (!SDL_SetTextureBlendMode(mTexture, mBlendMode))
        {
            SDL_Log("WARNING: RenderTexture::setBlendMode - Failed to set blend mode: %s", SDL_GetError());
        }
    }
}

SDL_BlendMode RenderTexture::getBlendMode() const noexcept
{
    return mBlendMode;
}

void RenderTexture::draw(SDL_Texture* texture, const SDL_FRect* srcRect, const SDL_FRect* dstRect)
{
    if (!isValid() || !mActive)
    {
        SDL_Log("ERROR: RenderTexture::draw - Render texture is not active");
        return;
    }

    if (!texture)
    {
        SDL_Log("ERROR: RenderTexture::draw - Invalid texture");
        return;
    }

    if (!SDL_RenderTexture(mRenderer, texture, srcRect, dstRect))
    {
        SDL_Log("ERROR: RenderTexture::draw - Failed to render texture: %s", SDL_GetError());
    }
}

