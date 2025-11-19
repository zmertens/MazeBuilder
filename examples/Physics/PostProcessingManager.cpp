#include "PostProcessingManager.hpp"

bool PostProcessingManager::initialize(SDL_Renderer* renderer, int width, int height)
{
    if (!renderer || width <= 0 || height <= 0)
    {
        SDL_Log("ERROR: PostProcessingManager::initialize - Invalid parameters");
        return false;
    }

    mRenderer = renderer;
    mWidth = width;
    mHeight = height;

    // Create scene texture (where the game renders to)
    mSceneTexture = std::make_unique<RenderTexture>(width, height, renderer);
    if (!mSceneTexture->isValid())
    {
        SDL_Log("ERROR: PostProcessingManager::initialize - Failed to create scene texture");
        return false;
    }

    // Create final texture (after post-processing)
    mFinalTexture = std::make_unique<RenderTexture>(width, height, renderer);
    if (!mFinalTexture->isValid())
    {
        SDL_Log("ERROR: PostProcessingManager::initialize - Failed to create final texture");
        return false;
    }

    // Create box blur effect
    mBoxBlur = std::make_unique<BoxBlurEffect>(2);
    if (!mBoxBlur->create(width, height, renderer))
    {
        SDL_Log("ERROR: PostProcessingManager::initialize - Failed to create box blur");
        return false;
    }

    // Create bloom effect
    mBloom = std::make_unique<BloomEffect>(0.7f, 3);
    if (!mBloom->create(width, height, renderer))
    {
        SDL_Log("ERROR: PostProcessingManager::initialize - Failed to create bloom");
        return false;
    }

    // Create post-processing pipeline
    mPipeline = std::make_unique<PostProcessingPipeline>();
    if (!mPipeline->create(width, height, renderer))
    {
        SDL_Log("ERROR: PostProcessingManager::initialize - Failed to create pipeline");
        return false;
    }

    SDL_Log("PostProcessingManager initialized successfully (%dx%d)", width, height);
    return true;
}

void PostProcessingManager::shutdown()
{
    if (mSceneActive)
    {
        endScene();
    }

    mPipeline.reset();
    mBloom.reset();
    mBoxBlur.reset();
    mFinalTexture.reset();
    mSceneTexture.reset();
    mRenderer = nullptr;
}

bool PostProcessingManager::beginScene()
{
    if (!isReady())
    {
        SDL_Log("ERROR: PostProcessingManager::beginScene - Not ready");
        return false;
    }

    if (mSceneActive)
    {
        SDL_Log("WARNING: PostProcessingManager::beginScene - Scene already active");
        return false;
    }

    if (!mSceneTexture->setActive(true))
    {
        SDL_Log("ERROR: PostProcessingManager::beginScene - Failed to activate scene texture");
        return false;
    }

    mSceneActive = true;
    return true;
}

void PostProcessingManager::endScene()
{
    if (!mSceneActive)
    {
        return;
    }

    mSceneTexture->display();
    mSceneTexture->setActive(false);
    mSceneActive = false;
}

void PostProcessingManager::present(SDL_Renderer* targetRenderer)
{
    if (!isReady())
    {
        return;
    }

    if (mSceneActive)
    {
        endScene();
    }

    // Apply post-processing effects if any are enabled
    bool anyEffectEnabled = mBoxBlurEnabled || mBloomEnabled;

    if (anyEffectEnabled)
    {
        applyEffects();

        // Render final result to target
        SDL_Texture* finalTex = mFinalTexture->getTexture();
        if (finalTex)
        {
            SDL_RenderTexture(targetRenderer, finalTex, nullptr, nullptr);
        }
    }
    else
    {
        // No effects, render scene texture directly
        SDL_Texture* sceneTex = mSceneTexture->getTexture();
        if (sceneTex)
        {
            SDL_RenderTexture(targetRenderer, sceneTex, nullptr, nullptr);
        }
    }
}

void PostProcessingManager::setBlurRadius(int radius)
{
    if (mBoxBlur)
    {
        mBoxBlur->setRadius(radius);
    }
}

void PostProcessingManager::setBlurPasses(int passes)
{
    if (mBoxBlur)
    {
        mBoxBlur->setPasses(passes);
    }
}

void PostProcessingManager::setBloomThreshold(float threshold)
{
    if (mBloom)
    {
        mBloom->setThreshold(threshold);
    }
}

void PostProcessingManager::setBloomIntensity(float intensity)
{
    if (mBloom)
    {
        mBloom->setIntensity(intensity);
    }
}

void PostProcessingManager::setBloomBlurRadius(int radius)
{
    if (mBloom)
    {
        mBloom->setBlurRadius(radius);
    }
}

SDL_Renderer* PostProcessingManager::getSceneRenderer() const
{
    if (mSceneTexture)
    {
        return mSceneTexture->getRenderer();
    }
    return mRenderer;
}

bool PostProcessingManager::isReady() const
{
    return mSceneTexture && mSceneTexture->isValid() &&
           mFinalTexture && mFinalTexture->isValid() &&
           mBoxBlur && mBoxBlur->isReady() &&
           mBloom && mBloom->isReady() &&
           mPipeline && mPipeline->isReady();
}

bool PostProcessingManager::resize(int width, int height)
{
    if (mSceneActive)
    {
        endScene();
    }

    mWidth = width;
    mHeight = height;

    // Recreate all render textures with new size
    if (!mSceneTexture->create(width, height, mRenderer))
    {
        SDL_Log("ERROR: PostProcessingManager::resize - Failed to resize scene texture");
        return false;
    }

    if (!mFinalTexture->create(width, height, mRenderer))
    {
        SDL_Log("ERROR: PostProcessingManager::resize - Failed to resize final texture");
        return false;
    }

    if (!mBoxBlur->create(width, height, mRenderer))
    {
        SDL_Log("ERROR: PostProcessingManager::resize - Failed to resize box blur");
        return false;
    }

    if (!mBloom->create(width, height, mRenderer))
    {
        SDL_Log("ERROR: PostProcessingManager::resize - Failed to resize bloom");
        return false;
    }

    if (!mPipeline->create(width, height, mRenderer))
    {
        SDL_Log("ERROR: PostProcessingManager::resize - Failed to resize pipeline");
        return false;
    }

    SDL_Log("PostProcessingManager resized to %dx%d", width, height);
    return true;
}

void PostProcessingManager::applyEffects()
{
    if (!isReady())
    {
        return;
    }

    // Build effect pipeline
    mPipeline->clearEffects();

    if (mBoxBlurEnabled && mBoxBlur)
    {
        mPipeline->addEffect(mBoxBlur);
    }

    if (mBloomEnabled && mBloom)
    {
        mPipeline->addEffect(mBloom);
    }

    // Apply effects
    mPipeline->apply(mSceneTexture->getTexture(), *mFinalTexture, mRenderer);
}

