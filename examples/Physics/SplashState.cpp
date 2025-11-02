#include "SplashState.hpp"

#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"

SplashState::SplashState(StateStack& stack, Context context)
    : State(stack, context)
    , mShowText{ true }
    , mSplashSprite{context.textures->get(Textures::ID::MAZE)} {

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