#include "GameState.hpp"

#include <functional>

#include "CommandQueue.hpp"
#include "Player.hpp"
#include "StateStack.hpp"
#include "Texture.hpp"

GameState::GameState(StateStack& stack, State::Context context)
    : State{stack, context}
    , mWorld{*context.window}
    , mPlayer{*context.player}
{
    mWorld.init();
}

void GameState::draw() const noexcept
{
    mWorld.draw();
}

bool GameState::update(float dt) noexcept
{
    mWorld.update(dt);
    
    auto& commands = mWorld.getCommandQueue();
    mPlayer.handleRealtimeInput(std::ref(commands));

    return true;
}

bool GameState::handleEvent(const SDL_Event& event) noexcept
{
    auto& commands = mWorld.getCommandQueue();

    mPlayer.handleEvent(event, std::ref(commands));

    if (event.type == SDL_EVENT_KEY_DOWN) {

        if (event.key.scancode == SDL_SCANCODE_ESCAPE) {

            // requestStackPush(States::ID::PAUSE);
            requestStackPop();
        }
    }

    return true;
}
