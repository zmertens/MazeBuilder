#include "SplashState.hpp"

#include <SDL3/SDL.h>

#include "LoadingState.hpp"
#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "StateStack.hpp"

SplashState::SplashState(StateStack& stack, Context context)
    : State(stack, context)
    , mShowText{ true }
    , mSplashSprite{context.textures->get(Textures::ID::SPLASH_SCREEN)} {

}

void SplashState::draw() const noexcept {

    auto& window = *getContext().window;
    
    window.draw(mSplashSprite);
}

bool SplashState::update(float dt) noexcept {

    return true;
}

bool SplashState::handleEvent(const SDL_Event& event) noexcept {

    if (event.type == SDL_EVENT_KEY_DOWN) {

        // Only allow transition if loading is complete
        if (!isLoadingComplete()) {
            SDL_Log("Loading not complete yet, please wait...");
            return true;
        }

        // Pop the splash state
        requestStackPop();
        // Pop the loading state underneath
        requestStackPop();
        // Push the game state
        requestStackPush(States::ID::GAME);
        mShowText = !mShowText;
    }

    return true;
}

bool SplashState::isLoadingComplete() const noexcept {
    
    // Check if the state below us (LoadingState) is finished
    LoadingState* loadingState = getStack().peekState<LoadingState*>();
    
    if (loadingState) {
        return loadingState->isFinished();
    }
    
    // If there's no LoadingState below, assume loading is complete
    return true;
}