#include "Player.hpp"

#include "CommandQueue.hpp"

#include <SDL3/SDL.h>

Player::Player() {

    mKeyBinding[SDL_SCANCODE_A] = Action::MOVE_LEFT;
    mKeyBinding[SDL_SCANCODE_D] = Action::MOVE_RIGHT;

    initializeActions();

    for (auto& pair : mActionBinding) {
        pair.second.category = Category::Type::PLAYER;
    }
}

void Player::handleEvent(const SDL_Event& event, CommandQueue& commands) {
    
    if (event.type == SDL_EVENT_KEY_DOWN) {
     
        auto found = mKeyBinding.find(event.key.scancode);
     
        if (found != mKeyBinding.cend() && !isRealtimeAction(found->second)) {
            
            commands.push(mActionBinding[found->second]);
        }
    }
}

// Handle continuous input for realtime actions
void Player::handleRealtimeInput(CommandQueue& commands) {
    
    const auto* keyState = SDL_GetKeyboardState(NULL);

    if (keyState[SDL_SCANCODE_LEFT]) {
        
    }
    if (keyState[SDL_SCANCODE_RIGHT]) {
        
    }
    if (keyState[SDL_SCANCODE_UP]) {
        
    }
    if (keyState[SDL_SCANCODE_DOWN]) {
        
    }
    
    if (keyState[SDL_SCANCODE_Q]) {
        
    }
    if (keyState[SDL_SCANCODE_E]) {
        
    }
    if (keyState[SDL_SCANCODE_EQUALS]) {
        
    }
    if (keyState[SDL_SCANCODE_MINUS]) {
        
    }
}

bool Player::isRealtimeAction(Action action)
{
    switch (action) {
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

	// mActionBinding[Action::MOVE_LEFT].action = derivedAction<Aircraft>(AircraftMover(-playerSpeed, 0.f));
	// mActionBinding[Action::MOVE_RIGHT].action = derivedAction<Aircraft>(AircraftMover(+playerSpeed, 0.f));
	// mActionBinding[Action::MOVE_UP].action = derivedAction<Aircraft>(AircraftMover(0.f, -playerSpeed));
	// mActionBinding[Action::MOVE_DOWN].action = derivedAction<Aircraft>(AircraftMover(0.f, +playerSpeed));
}

void Player::assignKey(Action action, std::uint32_t key) {

    // Remove all keys that already map to action
	for (auto itr = mKeyBinding.begin(); itr != mKeyBinding.end(); ){
		if (itr->second == action)
			mKeyBinding.erase(itr++);
		else
			++itr;
	}

	// Insert new binding
	mKeyBinding[key] = action;
}
std::uint32_t Player::getAssignedKey(Action action) const {
    
    for (const auto& pair : mKeyBinding) {

        if (pair.second == action) {

            return pair.first;
        }
    }
    
    return SDL_SCANCODE_UNKNOWN;
}