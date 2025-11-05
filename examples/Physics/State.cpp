#include "State.hpp"

#include "StateStack.hpp"

#include <SDL3/SDL.h>

State::State::Context::Context(RenderWindow& window, TextureManager& textures, Player& player)
    : window{&window}
      , textures{&textures}
      , player{&player}
{
}

State::State(StateStack& stack, Context context)
    : mStack{&stack}
      , mContext{context}
{
}

void State::requestStackPush(States::ID stateID)
{
    mStack->pushState(stateID);
}

void State::requestStackPop()
{
    mStack->popState();
}

void State::requestStateClear()
{
    mStack->clearStates();
}

State::Context State::getContext() const noexcept
{
    return mContext;
}

StateStack& State::getStack() const noexcept
{
    return *mStack;
}
