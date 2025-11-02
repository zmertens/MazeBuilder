#ifndef LOADING_STATE_HPP
#define LOADING_STATE_HPP

#include <string>
#include <string_view>
#include <unordered_map>

#include "Sprite.hpp"
#include "State.hpp"
#include "WorkerConcurrent.hpp"

class StateStack;
union SDL_Event;

class LoadingState : public State {
public:

    explicit LoadingState(StateStack& stack, State::Context context, std::string_view resourcePath = "");

    virtual void draw() const noexcept override;
    virtual bool update(float dt) noexcept override;
    virtual bool handleEvent(const SDL_Event& event) noexcept override;

    // Check if loading has completed
    bool isFinished() const noexcept;

private:
    void loadResources() noexcept;

    void loadTexturesFromResources(const std::unordered_map<std::string, std::string>& resources) noexcept;

    void setCompletion(float percent) noexcept;

    Sprite mLoadingSprite;

    WorkerConcurrent mForeman;
    
    bool mHasFinished;

    std::string mResourcePath;
};

#endif // LOADING_STATE_HPP
