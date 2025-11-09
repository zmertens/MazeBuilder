#include "StateStack.hpp"

#include <stdexcept>

#include <SDL3/SDL_events.h>

StateStack::StateStack(State::Context context)
    : mStack()
      , mPendingList()
      , mContext(context)
      , mFactories()
{
}

void StateStack::update(float dt) noexcept
{
    for (auto it = mStack.rbegin(); it != mStack.rend(); ++it)
    {
        if (!(*it)->update(dt))
        {
            break;
        }
    }

    applyPendingChanges();
}

void StateStack::draw() const noexcept
{
    if (mStack.empty())
    {
        return;
    }

    // Find the first opaque state from the top of the stack
    auto firstOpaque = mStack.begin();
    for (auto it = mStack.rbegin(); it != mStack.rend(); ++it)
    {
        if ((*it)->isOpaque())
        {
            // Convert reverse iterator to forward iterator
            // .base() returns an iterator one position after what the reverse iterator points to
            // So we need to decrement to get the actual opaque state
            firstOpaque = std::prev(it.base());
            break;
        }
    }

    // Draw from the first opaque state to the top
    for (auto it = firstOpaque; it != mStack.end(); ++it)
    {
        (*it)->draw();
    }
}

void StateStack::handleEvent(const SDL_Event& event) noexcept
{
    for (auto it = mStack.rbegin(); it != mStack.rend(); ++it)
    {
        if (!(*it)->handleEvent(event))
        {
            break;
        }
    }

    applyPendingChanges();
}

void StateStack::pushState(States::ID stateID)
{
    mPendingList.emplace_back(Action::PUSH, stateID);
}

void StateStack::popState()
{
    mPendingList.emplace_back(Action::POP);
}

void StateStack::clearStates()
{
    mPendingList.emplace_back(Action::CLEAR);
}

bool StateStack::isEmpty() const noexcept
{
    return mStack.empty();
}

State::Ptr StateStack::createState(States::ID stateID)
{
    if (auto found = mFactories.find(stateID); found != mFactories.cend())
    {
        return found->second();
    }

    throw std::runtime_error("StateStack::createState - No factory found for state ID");
}

void StateStack::applyPendingChanges()
{
    for (const PendingChange& change : mPendingList)
    {
        switch (change.action)
        {
        case Action::PUSH:
            mStack.push_back(createState(change.stateID));
            break;
        case Action::POP:
            mStack.pop_back();
            break;
        case Action::CLEAR:
            mStack.clear();
            break;
        }
    }

    mPendingList.clear();
}

StateStack::PendingChange::PendingChange(Action action, States::ID stateID)
    : action(action)
      , stateID(stateID)
{
}
