#include "Player.hpp"

#include "CommandQueue.hpp"
#include "Entity.hpp"
#include "Pathfinder.hpp"
#include "Wall.hpp"

#include <box2d/box2d.h>

#include <SDL3/SDL.h>

Player::Player() : mIsActive(true), mIsOnGround(false)
{
    mKeyBinding[SDL_SCANCODE_LEFT] = Action::MOVE_LEFT;
    mKeyBinding[SDL_SCANCODE_RIGHT] = Action::MOVE_RIGHT;
    mKeyBinding[SDL_SCANCODE_SPACE] = Action::JUMP;

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
            if (found->second == Action::JUMP && !mIsOnGround)
            {
                return; // do not jump if not on ground
            }
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
            int numKeys = 0;
            const auto* keyState = SDL_GetKeyboardState(&numKeys);

            if (keyState && pair.first < static_cast<std::uint32_t>(numKeys) && keyState[pair.first])
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
        return true;
    default:
        return false;
    }
}

void Player::initializeActions()
{
    static constexpr auto playerSpeed = 200.f;
    static constexpr auto jumpForce = -500.f;

    mActionBinding[Action::MOVE_LEFT].action = derivedAction<Pathfinder>(
        [](Pathfinder& pathfinder, float)
        {
            b2Body_ApplyForceToCenter(pathfinder.getBodyId(), {-playerSpeed, 0.f}, true);
        }
    );

    mActionBinding[Action::MOVE_RIGHT].action = derivedAction<Pathfinder>(
        [](Pathfinder& pathfinder, float)
        {
            b2Body_ApplyForceToCenter(pathfinder.getBodyId(), {+playerSpeed, 0.f}, true);
        }
    );

    mActionBinding[Action::JUMP].action = derivedAction<Pathfinder>(
        [this](Pathfinder& pathfinder, float)
        {
            if (mIsOnGround)
            {
                b2Body_ApplyLinearImpulseToCenter(pathfinder.getBodyId(), {0.f, jumpForce}, true);
                setGroundContact(false);
            }
        }
    );
}

void Player::assignKey(Action action, std::uint32_t key)
{
    // Remove all keys that already map to action
    for (auto it = mKeyBinding.begin(); it != mKeyBinding.end();)
    {
        if (it->second == action)
            it = mKeyBinding.erase(it);
        else
            ++it;
    }

    // Insert new binding
    mKeyBinding[key] = action;
}

std::uint32_t Player::getAssignedKey(Action action) const
{
    for (auto& pair : mKeyBinding)
    {
        if (pair.second == action)
            return pair.first;
    }

    return SDL_SCANCODE_UNKNOWN;
}

void Player::onBeginContact(Entity* other) noexcept
{
    if (const auto* wall = dynamic_cast<Wall*>(other))
    {
        if (wall->getOrientation() == Wall::Orientation::HORIZONTAL)
        {
            this->setGroundContact(true);
        }
    }
}

void Player::onEndContact(Entity* other) noexcept
{
    if (const auto* wall = dynamic_cast<Wall*>(other))
    {
        if (wall->getOrientation() == Wall::Orientation::HORIZONTAL)
        {
            this->setGroundContact(false);
        }
    }
}


void Player::setGroundContact(bool contact)
{
    mIsOnGround = contact;
}

bool Player::hasGroundContact() const
{
    return mIsOnGround;
}

bool Player::isActive() const noexcept
{
    return mIsActive;
}

void Player::setActive(bool active) noexcept
{
    mIsActive = active;
}
