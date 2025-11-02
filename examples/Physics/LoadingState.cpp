#include "LoadingState.hpp"

#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"

#include "StateStack.hpp"

LoadingState::LoadingState(StateStack& stack, State::Context context)
    : State(stack, context)
    , mLoadingSprite{context.textures->get(Textures::ID::MAZE)}
    , mForeman{}
    , mHasFinished{false} {

    mForeman.initThreads();
    // Start the background work with a max time (e.g., 5 seconds of simulated work)
    mForeman.generate(5.0f);
}

void LoadingState::draw() const noexcept {

    auto& window = *getContext().window;

    window.draw(mLoadingSprite);
}

bool LoadingState::update(float dt) noexcept {

    if (!mHasFinished && mForeman.isDone()) {
        mHasFinished = true;
        SDL_Log("Loading complete! Press any key to continue...");
    }

    if (!mHasFinished) {
        setCompletion(mForeman.getCompletion());
    }

    return true;
}

bool LoadingState::handleEvent(const SDL_Event& event) noexcept {

	// Don't handle events - let the SplashState on top handle them
	// This state just runs in the background showing progress
	return false; // Pass events through to states below
}

void LoadingState::setCompletion(float percent) noexcept {
    
    if (percent > 1.f) {
        percent = 1.f;
    }

    // Update loading sprite or progress bar based on percent
    SDL_Log("Loading progress: %.2f%%", percent * 100.f);
}