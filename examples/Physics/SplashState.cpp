#include "SplashState.hpp"

#include <SDL3/SDL.h>

#include "LoadingState.hpp"
#include "Player.hpp"
#include "ResourceIdentifiers.hpp"
#include "ResourceManager.hpp"
#include "StateStack.hpp"

SplashState::SplashState(StateStack& stack, Context context)
    : State(stack, context)
      , mShowText{true}
      , mSplashSprite{context.textures->get(Textures::ID::LEVEL_ONE)}
{
    getContext().player->setActive(false);
}

void SplashState::draw() const noexcept
{
    auto& window = *getContext().window;

    window.draw(mSplashSprite);
}

bool SplashState::update(float dt, unsigned int subSteps) noexcept
{
    return true;
}

bool SplashState::handleEvent(const SDL_Event& event) noexcept
{
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        // Only allow transition if loading is complete
        if (!isLoadingComplete())
        {
#if defined(MAZE_DEBUG)

            SDL_Log("Loading not complete yet, please wait...");
#endif

            return true;
        }

        // Pop the splash state
        requestStackPop();
        // Pop the loading state underneath
        requestStackPop();
        // Push the game state
        requestStackPush(States::ID::MENU);
        mShowText = !mShowText;
    }

    return true;
}

bool SplashState::isLoadingComplete() const noexcept
{
    // Check if the state below us (LoadingState) is finished
    if (const auto* loadingState = getStack().peekState<LoadingState*>())
    {
        return loadingState->isFinished();
    }

    // If there's no LoadingState below, assume loading is complete
    return true;
}
