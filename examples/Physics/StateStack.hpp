#ifndef STATE_STACK_HPP
#define STATE_STACK_HPP

#include <map>
#include <functional>
#include <memory>
#include <vector>

#include "State.hpp"

union SDL_Event;

class StateStack
{
public:
    enum class Action
    {
        PUSH,
        POP,
        CLEAR,
    };

public:
    explicit StateStack(State::Context context);

    virtual ~StateStack() = default;

    // Delete copy constructor and copy assignment operator
    // because StateStack contains std::unique_ptr which is not copyable
    StateStack(const StateStack&) = delete;
    StateStack& operator=(const StateStack&) = delete;

    // Allow move constructor and move assignment operator
    StateStack(StateStack&&) = default;
    StateStack& operator=(StateStack&&) = default;

    template <typename T>
    void registerState(States::ID stateID);

    template <typename T, typename ResourcePath>
    void registerState(States::ID stateID, ResourcePath&& resourcePath);

    void update(float dt, unsigned int subSteps) noexcept;

    void draw() const noexcept;

    void handleEvent(const SDL_Event& event) noexcept;

    void pushState(States::ID stateID);

    void popState();

    void clearStates();

    template <typename Pointer>
    Pointer peekState() const noexcept;

    bool isEmpty() const noexcept;

private:
    State::Ptr createState(States::ID stateID);
    void applyPendingChanges();

private:
    struct PendingChange
    {
        explicit PendingChange(Action action, States::ID stateID = States::ID::DONE);

        Action action;
        States::ID stateID;
    };

private:
    std::vector<State::Ptr> mStack;
    std::vector<PendingChange> mPendingList;
    State::Context mContext;
    std::map<States::ID, std::function<State::Ptr()>> mFactories;
};

template <typename T>
void StateStack::registerState(States::ID stateID)
{
    mFactories.insert_or_assign(stateID, [this]()
    {
        return State::Ptr(std::make_unique<T>(*this, mContext));
    });
}

template <typename T, typename ResourcePath>
void StateStack::registerState(States::ID stateID, ResourcePath&& resourcePath)
{
    mFactories.insert_or_assign(stateID, [this, resourcePath = std::forward<ResourcePath>(resourcePath)]()
    {
        return State::Ptr(std::make_unique<T>(*this, mContext, resourcePath));
    });
}

template <typename Pointer>
Pointer StateStack::peekState() const noexcept
{
    for (auto it = mStack.rbegin(); it != mStack.rend(); ++it)
    {
        if (auto statePtr = dynamic_cast<Pointer>((*it).get()))
        {
            return statePtr;
        }
    }

    return nullptr;
}

#endif // STATE_STACK_HPP
