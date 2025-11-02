#include "LoadingState.hpp"

#include <SDL3/SDL.h>

#include "JsonUtils.hpp"
#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "StateStack.hpp"

/// @brief 
/// @param stack 
/// @param context 
/// @param resourcePath ""
LoadingState::LoadingState(StateStack& stack, State::Context context, std::string_view resourcePath)
    : State(stack, context)
    , mLoadingSprite{context.textures->get(Textures::ID::MAZE)}
    , mForeman{}
    , mHasFinished{false} {

    mForeman.initThreads();
    
    // Start loading resources in background if path is provided
    if (!resourcePath.empty()) {

        loadResources(resourcePath);
    } else {

        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "LoadingState: No resource path provided: %s\n", resourcePath.data());
        mHasFinished = true;
    }
}

void LoadingState::draw() const noexcept {

    auto& window = *getContext().window;

    window.draw(mLoadingSprite);
}

bool LoadingState::update(float dt) noexcept {

    if (!mHasFinished && mForeman.isDone()) {
        
        // Loading is complete - get the loaded resources
        auto resources = mForeman.getResources();
        SDL_Log("Loading complete! Loaded %zu resources. Press any key to continue...\n", resources.size());
        
        // Now we could use these resources to load textures, etc.
        // For now, just mark as finished
        mHasFinished = true;
    }

    if (!mHasFinished) {

        setCompletion(mForeman.getCompletion());
    }

    return true;
}

bool LoadingState::handleEvent(const SDL_Event& event) noexcept {

	return true;
}

void LoadingState::setCompletion(float percent) noexcept {
    
    if (percent > 1.f) {
        percent = 1.f;
    }

    // Update loading sprite or progress bar based on percent
    SDL_Log("Loading progress: %.2f%%", percent * 100.f);
}

/// @brief Load resources from the specified path
/// @param resourcePath Path to the JSON resource configuration
void LoadingState::loadResources(std::string_view resourcePath) noexcept {

    SDL_Log("LoadingState::loadResources - Loading from: %s\n", resourcePath.data());
    
    // This would be called by the application to trigger resource loading
    // The resources would be loaded by the worker threads and stored
    mForeman.generate(resourcePath);
}