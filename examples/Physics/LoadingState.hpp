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

class LoadingState : public State
{
public:
    explicit LoadingState(StateStack& stack, State::Context context, std::string_view resourcePath = "");

    void draw() const noexcept override;
    bool update(float dt, unsigned int subSteps) noexcept override;
    bool handleEvent(const SDL_Event& event) noexcept override;

    // Check if loading has completed
    bool isFinished() const noexcept;

private:
    void loadResources() noexcept;

    void loadTexturesFromWorkerRequests() const noexcept;

    void loadMazeTexturesFromComposedStrings() const noexcept;

    void loadWindowIcon(const std::unordered_map<std::string, std::string>& resources) noexcept;

    void setCompletion(float percent) noexcept;

    Sprite mLoadingSprite;

    WorkerConcurrent mForeman;

    bool mHasFinished;

    std::string mResourcePath;
};

#endif // LOADING_STATE_HPP
