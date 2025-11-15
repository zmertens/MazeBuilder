#include "Player.hpp"

#include "CommandQueue.hpp"
#include "Pathfinder.hpp"

#include <SDL3/SDL.h>

Player::Player()
{
    mKeyBinding[SDL_SCANCODE_A] = Action::MOVE_LEFT;
    mKeyBinding[SDL_SCANCODE_D] = Action::MOVE_RIGHT;

    initializeActions();

    for (auto& pair : mActionBinding)
    {
        pair.second.category = Category::Type::PLAYER;
    }
}

void Player::handleEvent(const SDL_Event& event, CommandQueue& commands)
{
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        auto found = mKeyBinding.find(event.key.scancode);

        if (found != mKeyBinding.cend() && !isRealtimeAction(found->second))
        {
            commands.push(mActionBinding[found->second]);
        }
    }
}

// Handle continuous input for realtime actions
void Player::handleRealtimeInput(CommandQueue& commands)
{
    for (auto& pair : mKeyBinding)
    {
        if (isRealtimeAction(pair.second))
        {
            const auto* keyState = SDL_GetKeyboardState(nullptr);

            if (pair.first < SDL_arraysize(keyState) && keyState[pair.first])
            {
                commands.push(mActionBinding[pair.second]);
            }
        }
    }
}

bool Player::isRealtimeAction(Action action)
{
    switch (action)
    {
    case Action::MOVE_LEFT:
    case Action::MOVE_RIGHT:
    case Action::MOVE_UP:
    case Action::MOVE_DOWN:
        return true;
    default:
        return false;
    }
}

void Player::initializeActions()
{
    static constexpr auto playerSpeed = 200.f;

    // Define movement functor similar to SFML's AircraftMover
    // Lambda captures velocity and applies it to Pathfinder
    mActionBinding[Action::MOVE_LEFT].action = derivedAction<Pathfinder>(
        [](Pathfinder& pathfinder, float dt)
        {
            pathfinder.accelerate(-playerSpeed * dt, 0.f);
        }
    );

    mActionBinding[Action::MOVE_RIGHT].action = derivedAction<Pathfinder>(
        [](Pathfinder& pathfinder, float dt)
        {
            pathfinder.accelerate(+playerSpeed * dt, 0.f);
        }
    );

    mActionBinding[Action::MOVE_UP].action = derivedAction<Pathfinder>(
        [](Pathfinder& pathfinder, float dt)
        {
            pathfinder.accelerate(0.f, -playerSpeed * dt);
        }
    );

    mActionBinding[Action::MOVE_DOWN].action = derivedAction<Pathfinder>(
        [](Pathfinder& pathfinder, float dt)
        {
            pathfinder.accelerate(0.f, +playerSpeed * dt);
        }
    );
}

void Player::assignKey(Action action, std::uint32_t key)
{
    // Remove all keys that already map to action
    for (auto itr = mKeyBinding.begin(); itr != mKeyBinding.end();)
    {
        if (itr->second == action)
            mKeyBinding.erase(itr++);
        else
            ++itr;
    }

    // Insert new binding
    mKeyBinding[key] = action;
}

std::uint32_t Player::getAssignedKey(Action action) const
{
    for (const auto& pair : mKeyBinding)
    {
        if (pair.second == action)
        {
            return pair.first;
        }
    }

    return SDL_SCANCODE_UNKNOWN;
}

bool Player::isActive() const noexcept
{
    return mIsActive;
}

void Player::setActive(bool active) noexcept
{
    this->mIsActive = active;
}
