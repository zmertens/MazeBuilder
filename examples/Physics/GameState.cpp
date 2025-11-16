#include "GameState.hpp"

#include <SDL3/SDL.h>

#include <functional>

#include <MazeBuilder/create.h>

#include "CommandQueue.hpp"
#include "Player.hpp"
#include "StateStack.hpp"
#include "Texture.hpp"

GameState::GameState(StateStack& stack, Context context)
    : State{stack, context}
      , mWorld{*context.window, *context.fonts, *context.textures}
      , mPlayer{*context.player}
{
    mPlayer.setActive(true);
    mWorld.init();
    mWorld.setPlayer(context.player);
}

void GameState::draw() const noexcept
{
    mWorld.draw();
}

bool GameState::update(float dt, unsigned int subSteps) noexcept
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
    mWorld.handleEvent(event);

    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.key.scancode == SDL_SCANCODE_ESCAPE)
        {
            requestStackPush(States::ID::PAUSE);
        }
    }

    return true;
}
